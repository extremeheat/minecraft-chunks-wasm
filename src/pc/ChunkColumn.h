#pragma once
#include "../Block.h"
#include "../Registry.h"
#include "../Types.h"
#include "../mcutil/nbt.h"
#include "BiomeSection.h"
#include "ChunkSection.h"

const int SectionWidth = 16;
const int SectionHeight = 16;

class ChunkColumn {
 public:
  int worldHeight;
  ChunkSection sections[24];
  BiomeSection biomes[24];
  PalettedStorage<int> skyLights[24];
  PalettedStorage<int> blockLights[24];
  struct {
    BlockEntity *list;
    int count;
  } blockEntities;

  Registry *registry;
  // Chunk offset (to handle negative Y)
  int co;

  ChunkColumn(Registry *registry) {
    this->registry = registry;
    this->co = 4;

    for (int i = 0; i < 24; i++) {
      this->skyLights[i].init(4, 2048);
      this->blockLights[i].init(4, 2048);
    }
  }

  void initialize(int (*initFunction)(Vec3)) {
    // int x = 0, y = 0, z = 0;
    auto [x, y, z] = Vec3{0, 0, 0};
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
    // clang-format off
    return {
      this->getBlockStateId(pos),
      this->getBiomeId(pos),
      this->getBlockLight(pos), 
      this->getSkyLight(pos),
      this->getBlockEntity(pos)
    };
    // clang-format on
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

  int getBiomeId(const Vec3 &pos) {
    return this->biomes[co + (pos.y >> 4)].getBiomeId(pos);
  }

  int getBlockLight(const Vec3 &pos) {
    return this->blockLights[co + (pos.y >> 4)].getAt(pos.x, pos.y & 0xf, pos.z);
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
    this->blockLights[co + (pos.y >> 4)].setAt(pos.x, pos.y & 0xf, pos.z,
                                             blockLight);
  }

  void setSkyLight(const Vec3 &pos, int skyLight) {
    this->skyLights[co + (pos.y >> 4)].setAt(pos.x, pos.y & 0xf, pos.z, skyLight);
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

    u8 tempBuffer[max_size];
    BinaryStream stream(tempBuffer, max_size);

    int numberOfSections = 24;
    for (int i = 0; i < numberOfSections; i++) {
      this->sections[i].write(stream);
      this->biomes[i].write(stream);
    }

    return stream.write(buffer, bufferSize);
  }

  void fromNetworkSerialized(BinaryStream &stream) {
    int numberOfSections = 24;
    for (int i = 0; i < numberOfSections; i++) {
      this->sections[i].read(stream);
      this->biomes[i].read(stream);
    }
  }

  void fromNetworkSerialized(u8 *buffer, int bufferSize) {
    BinaryStream stream(buffer, bufferSize);
    this->fromNetworkSerialized(stream);
  }

  // https://wiki.vg/index.php?title=Protocol&oldid=17272#Chunk_Data_And_Update_Light
  void loadNetworkSerializedLights(u8 *skyLight, int skyLightLength,
                                   u8 *blockLight, int blockLightLength,
                                   u64 skyLightMask, u64 blockLightMask) {
    BinaryStream skyStream(skyLight, skyLightLength);
    BinaryStream blockStream(blockLight, blockLightLength);

    int numberOfSections = 24;
    int skyY = 0;
    for (int i = 0; i < numberOfSections; i++) {
      auto sectionMask = (1 << (i - 1));
      if (skyLightMask & sectionMask) {
        auto skyLightForSection = skyLight[2048 * skyY++];
        this->skyLights[i].read(skyStream);
      }
      if (blockLightMask & sectionMask) {
        auto blockLightForSection = blockLight[2048 * i];
        this->blockLights[i].read(blockStream);
      }
    }
  }

  static ChunkColumn *readChunkPacket(Registry *registry, u8 *buffer, int len) {
    BinaryStream stream(buffer, len);

    int x = stream.readIntBE();
    int z = stream.readIntBE();
    auto heightmaps = skipNBT(stream);
    if (!heightmaps) {
      // nbt reading error
      return reinterpret_cast<ChunkColumn *>(-1);
    }

    ChunkColumn *chunk = new ChunkColumn(registry);

    auto chunkPayloadSize = stream.readVarInt();
    chunk->fromNetworkSerialized(stream);

    auto blockEntitiesCount = stream.readVarInt();
    for (int i = 0; i < blockEntitiesCount; i++) {
      auto sXZ = stream.readByte();
      auto sY = stream.readShortBE();
      auto blockEntityType = stream.readVarInt();
      auto blockEntityNbt = skipNBT(stream);
    }
    auto trustEdges = stream.readByte();

    u64 skyLightMask = 0, blockLightMask = 0;

    auto skyLightMaskLen = stream.readVarInt();
    if (skyLightMask) {
      skyLightMask = stream.readLongBE();
    }

    auto blockLightMaskLen = stream.readVarInt();
    if (blockLightMask) {
      blockLightMask = stream.readLongBE();
    }

    auto emptySkyLightMaskLen = stream.readVarInt();
    if (emptySkyLightMaskLen) {
      stream.readLongBE();
    }
    auto emptyBlockLightMaskLen = stream.readVarInt();
    if (emptyBlockLightMaskLen) {
      stream.readLongBE();
    }

    u8 skylight[2048 * 24];
    u8 blocklight[2048 * 24];

    auto skyLightLength = stream.readVarInt();
    for (int i = 0; i < skyLightLength; i++) {
      stream.read(&skylight[2048 * i], 2048);
    }
    auto blockLightLength = stream.readVarInt();
    for (int i = 0; i < blockLightLength; i++) {
      stream.read(&blocklight[2048 * i], 2048);
    }

    chunk->loadNetworkSerializedLights(skylight, skyLightLength, blocklight,
                                       blockLightLength, skyLightMask,
                                       blockLightMask);

    return chunk;
  }
};
