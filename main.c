#define SEARCH_RADIUS 1

#include "finders.h"
#include "generator.h"
#include "layers.h"

#include <unistd.h>

int isViableMansion(LayerStack *g, int *map, int64_t seed, int regX, int regZ) {
    Pos posMansion = getLargeStructurePos(MANSION_CONFIG, seed, regX, regZ);
    return isViableMansionPos(*g, map, posMansion.x, posMansion.z);
}

int isViableDesert(LayerStack *g, int *map, int64_t seed, int regX, int regZ) {
    Pos posDesert = getStructurePos(DESERT_PYRAMID_CONFIG, seed, regX, regZ);
    return isViableFeaturePos(Desert_Pyramid, *g, map, posDesert.x, posDesert.z);
}

int isViableWitchHut(LayerStack *g, int *map, int64_t seed, int regX, int regZ) {
    Pos posDesert = getStructurePos(SWAMP_HUT_CONFIG, seed, regX, regZ);
    return isViableFeaturePos(Swamp_Hut, *g, map, posDesert.x, posDesert.z);
}

int isViableMonument(LayerStack *g, int *map, int64_t seed, int regX, int regZ) {
    Pos posMonument = getLargeStructurePos(MONUMENT_CONFIG, seed, regX, regZ);
    return (isViableOceanMonumentPos(*g, map, posMonument.x, posMonument.z));
}

int isViableIgloo(LayerStack *g, int *map, int64_t seed, int regX, int regZ) {
    Pos posIgloo = getStructurePos(IGLOO_CONFIG, seed, regX, regZ);
    return (isViableFeaturePos(Igloo, *g, map, posIgloo.x, posIgloo.z));
}

int isViableVillage(LayerStack *g, int *map, int64_t seed, int regX, int regZ) {
    Pos posVillage = getStructurePos(VILLAGE_CONFIG, seed, regX, regZ);
    return (isViableVillagePos(*g, map, posVillage.x, posVillage.z));
}

int isViableInRadius(int (*isViable)(LayerStack *g, int *map, int64_t seed, int regX, int regZ), LayerStack *g, int *map, int64_t seed) {
    int i = 0;
    for (int regX = -SEARCH_RADIUS; regX <= SEARCH_RADIUS; regX++) {
        for (int regZ = -SEARCH_RADIUS; regZ <= SEARCH_RADIUS; regZ++) {
            if ((*isViable)(g, map, seed, regX, regZ)) {
                i++;
            }
        }
    }
    return i > 0;
}

void RSGfinder() {
    initBiomes();
    LayerStack g = setupGenerator(MC_1_12);
    int *map = allocCache(&g.layers[g.layerNum - 1], 256, 256);
    FILE *file = fopen("save.txt", "w");
    for (int64_t seed = 0; seed < 8000000000000; seed++) {
        applySeed(&g, seed);
        if (!(seed % 10000)) {
            printf("Currently at %ld\n", seed);
            fflush(file);
        }
        if (!isViableInRadius(isViableMansion, &g, map, seed)) continue;
        if (!isViableInRadius(isViableMonument, &g, map, seed)) continue;
        if (!isViableInRadius(isViableDesert, &g, map, seed)) continue;
        if (!isViableInRadius(isViableVillage, &g, map, seed)) continue;
        if (!isViableInRadius(isViableIgloo, &g, map, seed)) continue;
        printf("perfect seed : %ld \n", seed);
        fprintf(file, "%ld\n", seed);


    }
    fclose(file);
}


enum CORNER_POS {
    UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT
};

int isInCorner(StructureConfig structureConfig, int64_t seed, int x, int z, int CORNER_POS,long CORNER_OFFSET) {
    Pos p = getLargeStructureChunkInRegion(structureConfig, seed, x, z);
    switch (CORNER_POS) {
        case UP_RIGHT:
            if (p.x <= 0 + CORNER_OFFSET && p.z >= 31 - CORNER_OFFSET) {
                return 1;
            }
            return 0;
        case UP_LEFT:
            if (p.x >= 31 - CORNER_OFFSET && p.z >= 31 - CORNER_OFFSET) {
                return 1;
            }
            return 0;
        case DOWN_RIGHT:
            if (p.x <= 0 + CORNER_OFFSET && p.z < 0 + CORNER_OFFSET) {
                return 1;
            }
            return 0;
        case DOWN_LEFT:
            if (p.z <= 0 + CORNER_OFFSET && p.x >= 31 - CORNER_OFFSET) {
                return 1;
            }
            return 0;
        default:
            return 0;
    }
}

int isInCornerSimple(StructureConfig structureConfig, int64_t seed, int x, int z, int CORNER_POS,long CORNER_OFFSET) {
    Pos p = getStructureChunkInRegion(structureConfig, seed, x, z);
    switch (CORNER_POS) {
        case UP_RIGHT:
            if (p.x <= 0 + CORNER_OFFSET && p.z >= 31 - CORNER_OFFSET) {
                return 1;
            }
            return 0;
        case UP_LEFT:
            if (p.x >= 31 - CORNER_OFFSET && p.z >= 31 - CORNER_OFFSET) {
                return 1;
            }
            return 0;
        case DOWN_RIGHT:
            if (p.x <= 0 + CORNER_OFFSET && p.z < 0 + CORNER_OFFSET) {
                return 1;
            }
            return 0;
        case DOWN_LEFT:
            if (p.z <= 0 + CORNER_OFFSET && p.x >= 31 - CORNER_OFFSET) {
                return 1;
            }
            return 0;
        default:
            return 0;
    }
}

void QuadMonumentFinder(int64_t seed, int radius,long corner_offset) {
    initBiomes();
    LayerStack g = setupGenerator(MC_1_12);
    int *map = allocCache(&g.layers[g.layerNum - 1], 256, 256);
    FILE *file = fopen("save.txt", "w");
    Pos p;
    applySeed(&g, seed);
    int count = 0;
    for (int x = -radius; x < radius; x++) {
        for (int z = -radius; z < radius; z++) {
            count = 0;
            if (isInCorner(MONUMENT_CONFIG, seed, x, z, UP_LEFT,corner_offset) && isViableMonument(&g, map, seed, x , z))count++;
            if (isInCorner(MONUMENT_CONFIG, seed, x + 1, z, UP_RIGHT,corner_offset) && isViableMonument(&g, map, seed, x + 1, z)) count++;
            if (isInCorner(MONUMENT_CONFIG, seed, x, z + 1, DOWN_LEFT,corner_offset) && isViableMonument(&g, map, seed, x , z+1)) count++;
            if (isInCorner(MONUMENT_CONFIG, seed, x + 1, z + 1, DOWN_RIGHT,corner_offset) && isViableMonument(&g, map, seed, x + 1, z+1)) count++;
            if (count > 1) {
                p = getLargeStructurePos(MONUMENT_CONFIG, seed, x, z);
                printf("%d,%d  : %d\n", p.x, p.z,count);
                fprintf(file,"%d,%d  //count: %d\n", p.x, p.z,count);
                fflush(file);
            }
        }
    }


    //search4TriBases("./quadMonuments", 8, DESERT_PYRAMID_CONFIG, 1);
}

void doubleWitchHutFinder(int64_t seed, int radius,long corner_offset){
    initBiomes();
    LayerStack g = setupGenerator(MC_1_13);
    int *map = allocCache(&g.layers[g.layerNum - 1], 256, 256);
    FILE *file = fopen("save.txt", "w");
    Pos p;
    applySeed(&g, seed);
    int count = 0;
    for (int x = -radius; x < radius; x++) {
        for (int z = -radius; z < radius; z++) {
            count = 0;
            if (isInCornerSimple(FEATURE_CONFIG, seed, x, z, UP_LEFT,corner_offset) && isViableWitchHut(&g, map, seed, x , z))count++;
            if (isInCornerSimple(FEATURE_CONFIG, seed, x + 1, z, UP_RIGHT,corner_offset) && isViableWitchHut(&g, map, seed, x + 1, z)) count++;
            if (isInCornerSimple(FEATURE_CONFIG, seed, x, z + 1, DOWN_LEFT,corner_offset) && isViableWitchHut(&g, map, seed, x , z+1)) count++;
            if (isInCornerSimple(FEATURE_CONFIG, seed, x + 1, z + 1, DOWN_RIGHT,corner_offset) && isViableWitchHut(&g, map, seed, x + 1, z+1)) count++;
            if (count > 1) {
                p = getStructurePos(FEATURE_CONFIG, seed, x, z);
                printf("%d,%d  : %d\n", p.x, p.z,count);
                fprintf(file,"%d,%d  //count: %d\n", p.x, p.z,count);
                fflush(file);
            }
        }
    }
}

int main(int argc,char*argv[]) {
    if (argc!=4){
        printf("Usage is MonumentFinder.exe <seed> <radiusInchunks> <chunk offset distance from AFK point to center of monument (usually 5 Max: 8)>\n");
    }
    else{
        doubleWitchHutFinder( strtoll(argv[1], NULL, 0), (int)strtol(argv[2], NULL, 0),strtol(argv[3], NULL,0));
    }

}