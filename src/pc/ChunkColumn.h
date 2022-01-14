#include "../Block.h"
#include "../Registry.h"
#include "../Types.h"
#include "BiomeSection.h"
#include "ChunkSection.h"

const int SectionWidth = 16;
const int SectionHeight = 16;

class ChunkColumn {
 public:
  int worldHeight;
  ChunkSection sections[24];
  BiomeSection biomes[24];
  PalettedStorage<int> blockLights;
  struct {
    BlockEntity *list;
    int count;
  } blockEntities;

  Registry *registry;
  // Chunk offset (to handle negative Y)
  int co;

  ChunkColumn(Registry *registry) : blockLights(4, 16 * 16 * 16 * 24) {
    this->registry = registry;
    this->co = 4;
  }

  void initialize(int (*initFunction)(Vec3)) {
    // int x = 0, y = 0, z = 0;
    auto &[x, y, z] = Vec3{0, 0, 0};
    for (y = 0; y < this->worldHeight; y++) {
      for (z = 0; z < SectionWidth; z++) {
        for (x = 0; x < SectionWidth; x++) {
          auto block = initFunction({x, y, z});
          if (block != -1) {
            this->setBlockStateId({x, y, z}, block);
          }
        }
      }
    }
  }

  ChunkSection getChunkSection(int chunkY) {
    return this->sections[co + chunkY];
  }

  BiomeSection getBiomeSection(int chunkY) { return this->biomes[co + chunkY]; }

  Block getFullBlock(const Vec3 &pos) {
    return {this->getBlockStateId(pos), this->getBiomeId(pos),
            this->getBlockLight(pos), this->getSkyLight(pos),
            this->getBlockEntity(pos)};
  }

  void setBlock(const Vec3 &pos, Block &block) {
    this->setBlockStateId(pos, block.stateId);
    this->setBiomeId(pos, block.biomeId);
    this->setBlockLight(pos, block.blockLight);
    this->setSkyLight(pos, block.skyLight);
    if (block.blockEntity) {
      this->setBlockEntity(pos, BlockEntity(block.blockEntity));
    } else if (this->hasBlockEntity(pos)) {
      this->removeBlockEntity(pos);
    }
  }

  int getBlockStateId(const Vec3 &pos) { return 0; }

  int getBiomeId(const Vec3 &pos) { return 0; }

  int getBlockLight(const Vec3 &pos) {
    return this->blockLights.getAt(pos.x, pos.y, pos.z);
  }

  int getSkyLight(const Vec3 &pos) { return 0; }

  BlockEntity *getBlockEntity(const Vec3 &pos) {
    for (int i = 0; i < this->blockEntities.count; i++) {
      auto &blockEntity = this->blockEntities.list[i];
      if (blockEntity.position.x == pos.x && blockEntity.position.y == pos.y &&
          blockEntity.position.z == pos.z) {
        return &blockEntity;
      }
    }
    return nullptr;
  }

  bool hasBlockEntity(const Vec3 &pos) {
    for (int i = 0; i < this->blockEntities.count; i++) {
      auto &blockEntity = this->blockEntities.list[i];
      if (blockEntity.position.x == pos.x && blockEntity.position.y == pos.y &&
          blockEntity.position.z == pos.z) {
        return true;
      }
    }
    return false;
  }

  void removeBlockEntity(const Vec3 &pos) {
    auto newList = (BlockEntity *)malloc(sizeof(BlockEntity) *
                                         (this->blockEntities.count - 1));
    for (int i = 0, j = 0; i < this->blockEntities.count; i++) {
      auto &blockEntity = this->blockEntities.list[i];
      if (blockEntity.position == pos) {
        newList[j++] = blockEntity;
      }
    }
    free(this->blockEntities.list);
    this->blockEntities.list = newList;
    this->blockEntities.count--;
  }

  void setBlockStateId(const Vec3 &pos, int stateId) {
    auto section = this->getChunkSection(pos.y >> 4);
    section.setBlockStateId({pos.x, pos.y & 0xf, pos.z}, stateId);
  }

  void setBiomeId(const Vec3 &pos, int biomeId) {
    auto section = this->getBiomeSection(pos.y >> 4);
    section.setBiomeId({pos.x, pos.y & 0xf, pos.z}, biomeId);
  }

  void setBlockLight(const Vec3 &pos, int blockLight) {
    this->blockLights.setAt(pos.x, pos.y, pos.z, blockLight);
  }

  void setSkyLight(const Vec3 &pos, int skyLight) {
    // TODO: Implement
  }

  void setBlockEntity(const Vec3 &pos, BlockEntity blockEntity) {
    for (int i = 0; i < this->blockEntities.count; i++) {
      if (this->blockEntities.list[i].position == pos) {
        this->blockEntities.list[i] = blockEntity;
        return;
      }
    }
    this->blockEntities.list = (BlockEntity *)realloc(
        this->blockEntities.list,
        sizeof(BlockEntity) * (this->blockEntities.count + 1));
  }

  void toNetworkSerialized(out u8 *buffer, out int bufferSize) {
    // This may seem expensive, but it's really cheap. We allocate the max size
    // possible on a CC on the stack (which is just moving stack pointer) then
    // we copy it over to the heap with the known size.
    const int max_size = sizeof(ChunkColumn) + sizeof(ChunkSection) * 24 +
                         sizeof(BiomeSection) * 24 +
                         (8 * 8 * 8 * 24) /* blockLights */ +
                         sizeof(BlockEntity) * (16 * 16 * 16 * 24);

    u8 buffer[max_size];
    BinaryStream stream(buffer, max_size);

    int numberOfSections = 24;
    for (int i = 0; i < numberOfSections; i++) {
      this->sections[i].write(stream);
      this->biomes[i].write(stream);
    }

    return stream.write(buffer, bufferSize);
  }

  void fromNetworkSerialized(u8 *buffer, int bufferSize) {
    BinaryStream stream(buffer, bufferSize);

    int numberOfSections = 24;
    for (int i = 0; i < numberOfSections; i++) {
      this->sections[i].read(stream);
      this->biomes[i].read(stream);
    }
  }

  void loadNetworkSerializedLights(u8 *skyLight, int skyLightLength,
                                   u8 *blockLight, int blockLightLength,
                                   u64 skyLightMask, u64 blockLightMask) {
    BinaryStream skyStream(skyLight, skyLightLength);
    BinaryStream blockStream(blockLight, blockLightLength);

    int numberOfSections = 24;
    for (int i = 0; i < numberOfSections; i++) {
      this->blockLights.read(blockStream);
    }
  }

  // static ChunkColumn* fromSerialized() {

  // }

  // BinaryStream toSerialized() {
  //   // This may seem expensive, but it's really cheap. We allocate the max
  //   size possible on a CC on the
  //   // stack (which is just moving stack pointer) then we copy it over to the
  //   heap with the known size. const int max_size = sizeof(ChunkColumn) +
  //   sizeof(ChunkSection) * 24 + sizeof(BiomeSection) * 24 + (8*8*8*24) /*
  //   blockLights */ + sizeof(BlockEntity) * (16*16*16*24);

  //   u8 buffer[max_size];
  //   BinaryStream stream(buffer, max_size);
  //   for (int i = 0; i < 24; i++) {
  //     auto section = this->sections[i];
  //     if (!section.isEmpty()) {
  //       stream.writeByte(1);
  //       section.write(stream);
  //     }
  //   }
  //   for (int i = 0; i < 24; i++) {
  //     auto section = this->biomes[i];
  //     stream.write(section.toSerialized());
  //   }
  //   return stream;
  // }
};