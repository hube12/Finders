/**
 * This is an example program that demonstrates how to find seeds with a
 * quad-witch-hut located around the specified region (512x512 area).
 *
 * It uses some optimisations that cause it miss a small number of seeds, in
 * exchange for a major speed upgrade. (~99% accuracy, ~1200% speed)
 */

#include "finders.h"
#include "generator.h"
#include "layers.h"
#include <unistd.h>

FILE *file;

int hasStronghold(LayerStack *g, int *cache, int64_t seed, int maxDistance,int ismoo[]);

int main(int argc, char *argv[]) {

    // Always initialize the biome list before starting any seed finder or
    // biome generator.
    initBiomes();
    LayerStack g;

    // Translate the positions to the desired regions.
    int regPosX = 0;
    int regPosZ = 0;

    int mcversion = 0;
    const char *seedFileName;
    StructureConfig featureConfig;
    static int isMoo[256];
    unsigned long o;
    static const int moo[] ={mushroomIsland,mushroomIslandShore};
    if (!isMoo[moo[0]]) {
        for (o = 0; o < sizeof(moo) / sizeof(int); o++) {
            isMoo[moo[o]] = 1;
        }
    }
    mcversion = 113;

    regPosX -= 1;
    regPosZ -= 1;
    int64_t hits = 0;
    if (0) {
        featureConfig = SWAMP_HUT_CONFIG;
        seedFileName = "./seeds/quadhutbases_1_13_Q1.txt";
        // setupGeneratorMC113() biome generation is slower and unnecessary.
        // We are only interested in the biomes on land, which haven't changed
        // since MC 1.7 except for some modified variants.
        g = setupGeneratorMC17();
        // Use the 1.13 Hills layer to get the correct modified biomes.z
        g.layers[L_HILLS_64].getMap = mapHills113;
    } else {
        featureConfig = FEATURE_CONFIG;
        seedFileName = "./seeds/quadhutbases_1_7_Q1.txt";
        g = setupGeneratorMC17();
    }

    if (access(seedFileName, F_OK)) {
        printf("Seed base file does not exist: Creating new one.\n"
               "This may take a few minutes...\n");
        int threads = 6;
        int quality = 1;
        search4QuadBases(seedFileName, threads, featureConfig, quality);
    }

    int64_t i, j, qhcnt;
    int64_t base, seed;
    int64_t *qhcandidates = loadSavedSeeds(seedFileName, &qhcnt);


    Layer *lFilterBiome = &g.layers[L_BIOME_256];
    int *biomeCache = allocCache(lFilterBiome, 3, 3);

    // Load the positions of the four structures that make up the quad-structure
    // so we can test the biome at these positions.
    Pos qhpos[4];

    // Setup a dummy layer for Layer 19: Biome.
    Layer layerBiomeDummy;
    setupLayer(256, &layerBiomeDummy, NULL, 200, NULL);


    int areaX = (regPosX << 1) + 1;
    int areaZ = (regPosZ << 1) + 1;
    LayerStack we=setupGenerator(MC_1_12);
    int *maps = allocCache(&we.layers[we.layerNum - 1], 256, 256);

    // Search for a swamp at the structure positions
    for (i = 100; i < qhcnt; i++) {
        base = moveStructure(qhcandidates[i], regPosX, regPosZ);

        qhpos[0] = getStructurePos(featureConfig, base, 0 + regPosX, 0 + regPosZ);
        qhpos[1] = getStructurePos(featureConfig, base, 0 + regPosX, 1 + regPosZ);
        qhpos[2] = getStructurePos(featureConfig, base, 1 + regPosX, 0 + regPosZ);
        qhpos[3] = getStructurePos(featureConfig, base, 1 + regPosX, 1 + regPosZ);

        /*
        for(j = 0; j < 4; j++)
        {
            printf("(%d,%d) ", qhpos[j].x, qhpos[j].z);
        }
        printf("\n");
        */

        // This little magic code checks if there is a meaningful chance for
        // this seed base to generate swamps in the area.
        // The idea is that the conversion from Lush temperature to swampland is
        // independent of surroundings, so we can test the conversion
        // beforehand. Furthermore biomes tend to leak into the negative
        // coordinates because of the Zoom layers, so the majority of hits will
        // occur when SouthEast corner (at a 1:256 scale) of the quad-hut has a
        // swampland. (This assumption misses about 1 in 500 quad-hut seeds.)
        // Finally, here we also exploit that the minecraft random number
        // generator is quite bad, such that for the "mcNextRand() mod 6" check
        // it has a period pattern of ~3 on the high seed-bits.
        for (j = 0; j < 5; j++) {
            seed = base + ((j + 0x53) << 48);
            setWorldSeed(&layerBiomeDummy, seed);
            setChunkSeed(&layerBiomeDummy, areaX + 1, areaZ + 1);
            if (mcNextInt(&layerBiomeDummy, 6) == 5)
                break;
        }
        if (j >= 5) continue;


        int64_t swpc;

        for (j = 0; j < 0x10000; j++) {
            seed = base + (j << 48u);
            /** Pre-Generation Checks **/
            // We can check that at least one swamp could generate in this area
            // before doing the biome generator checks.
            setWorldSeed(&layerBiomeDummy, seed);

            setChunkSeed(&layerBiomeDummy, areaX + 1, areaZ + 1);
            if (mcNextInt(&layerBiomeDummy, 6) != 5)
                continue;

            // This seed base does not seem to contain many quad huts, so make
            // a more detailed analysis of the surroundings and see if there is
            // enough potential for more swamps to justify searching further.
            if (hits == 0 && (j & 0xfff) == 0xfff) {
                swpc = 0;
                setChunkSeed(&layerBiomeDummy, areaX, areaZ + 1);
                swpc += mcNextInt(&layerBiomeDummy, 6) == 5;
                setChunkSeed(&layerBiomeDummy, areaX + 1, areaZ);
                swpc += mcNextInt(&layerBiomeDummy, 6) == 5;
                setChunkSeed(&layerBiomeDummy, areaX, areaZ);
                swpc += mcNextInt(&layerBiomeDummy, 6) == 5;

                if (swpc < (j > 0x1000 ? 2 : 1)) break;
            }

            // Dismiss seeds that don't have a swamp near the quad temple.
            setWorldSeed(lFilterBiome, seed);
            genArea(lFilterBiome, biomeCache, (regPosX << 1) + 2, (regPosZ << 1) + 2, 1, 1);

            if (biomeCache[0] != swampland)
                continue;

            applySeed(&g, seed);
            if (getBiomeAtPos(g, qhpos[0]) != swampland) continue;
            if (getBiomeAtPos(g, qhpos[1]) != swampland) continue;
            if (getBiomeAtPos(g, qhpos[2]) != swampland) continue;
            if (getBiomeAtPos(g, qhpos[3]) != swampland) continue;

/*

            Pos posSpawn = getSpawn(MC_1_12, &we, maps, seed,1024);



            if (areBiomesViable(we,maps,posSpawn.x,posSpawn.z,200,isMoo)){
                Pos posMonument=getStructurePos(MONUMENT_CONFIG,seed,posSpawn.x/MONUMENT_CONFIG.regionSize,posSpawn.z/MONUMENT_CONFIG.regionSize);
                if (isViableOceanMonumentPos(we,maps,posMonument.x,posMonument.z)){
                    file = fopen("save.txt", "a");
                    fprintf(file, "seed: %"PRId64"\n", seed);
                    fclose(file);
                    printf("seed: %"PRId64"\n",seed);
                }

            }*/

            applySeed(&we, seed);
            hasStronghold(&we, maps, seed, 2500,isMoo);
            if (!(hits%1000)){
                printf("%ld hits \n",hits);
            }
            hits++;
        }
    }

    free(biomeCache);
    freeGenerator(g);

    return 0;
}

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int hasStronghold(LayerStack *g, int *cache, int64_t seed, int maxDistance,int isMoo[]) {
    Pos strongholds[128];
    // Approximateish center of quad hut formation.

    Pos pos = getSpawn(MC_1_12, g, cache, seed,1024);
    // printf("%d, %d \n",pos.x,pos.z);

    int cx = pos.x;
    int cz = pos.z;
    if (!areBiomesViable(*g,cache,cx,cz,150,isMoo)){
        return 0;
    }
    else{
        printf("Mooshroom ok\n");
    }
    // Quit searching strongholds in outer rings; include a fudge factor.
    int maxRadius = (int) round(sqrt(cx * cx + cz * cz)) + maxDistance + 16;

    int count = findStrongholds(MC_1_12, g, cache, strongholds, seed, 0, maxRadius);
    printf("%d\n",count);
    for (int i = 0; i < count; i++) {
        int dx = strongholds[i].x - cx;
        int dz = strongholds[i].z - cz;
        if (dx * dx + dz * dz <= maxDistance * maxDistance) {
            file = fopen("save.txt", "a");
            fprintf(file, "distance %f seed: %"PRId64"\n", sqrt(dx * dx + dz * dz), seed);
            fclose(file);
            printf("distance %f seed: %"PRId64"\n", sqrt(dx * dx + dz * dz), seed);
        }

    }

    return 0;
}