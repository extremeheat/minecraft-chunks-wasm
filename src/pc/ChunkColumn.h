#pragma once
#include "../Block.h"
#include "../Registry.h"
#include "../Types.h"
#include "../mcutil/nbt.h"
#include "BiomeSection.h"
#include "ChunkSection.h"
#ifndef WEBASSEMBLY
#include <new>
#endif

const int SectionWidth = 16;
const int SectionHeight = 16;

const int NUM_SECTIONS = 24;

#define DEBUG_LOG printf

class ChunkColumn {
 public:
  ChunkSection sections[NUM_SECTIONS];
  BiomeSection biomes[NUM_SECTIONS];
  PalettedStorage<int> skyLights[NUM_SECTIONS];
  PalettedStorage<int> blockLights[NUM_SECTIONS];

  struct {
    BlockEntity *list = 0;
    int count = 0;
  } blockEntities;

  Registry *registry;
  // Chunk offset (to handle negative Y)
  int co;
  int numSections;

  long blockLightMask = 0;
  long skyLightMask = 0;

  int minY = -4 << 4;
  int maxY = 20 << 4;
  int x;
  int z;

  ChunkColumn(Registry *registry, int x = 0, int z = 0) {
    this->registry = registry;
    this->x = x;
    this->z = z;
    this->co = 4;
    this->numSections = NUM_SECTIONS;

    for (int i = 0; i < NUM_SECTIONS; i++) {
      this->skyLights[i].init(4, 4096);
      this->blockLights[i].init(4, 4096);
    }
  }

  void initialize(int (*initFunction)(Vec3i)) {
    // int x = 0, y = 0, z = 0;
    auto [x, y, z] = Vec3i{0, 0, 0};
    for (y = minY; y < maxY; y++) {
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

  Block getFullBlock(const Vec3i &pos) {
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

  void setBlock(const Vec3i &pos, Block &block) {
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

  int getBlockStateId(const Vec3i &pos) {
    auto section = this->getChunkSection(pos.y >> 4);
    return section.getBlockStateId({pos.x, pos.y & 0xf, pos.z});
  }

  int getBiomeId(const Vec3i &pos) {
    return this->biomes[co + (pos.y >> 4)].getBiomeId(pos);
  }

  int getBlockLight(const Vec3i &pos) {
    return this->blockLights[co + (pos.y >> 4)].get(getLightIndex(pos));
  }

  int getSkyLight(const Vec3i &pos) {
    return this->skyLights[co + (pos.y >> 4)].get(getLightIndex(pos));
  }

  BlockEntity *getBlockEntity(const Vec3i &pos) {
    for (int i = 0; i < this->blockEntities.count; i++) {
      auto &blockEntity = this->blockEntities.list[i];
      if (blockEntity.position == pos) {
        return &blockEntity;
      }
    }
    return nullptr;
  }

  bool hasBlockEntity(const Vec3i &pos) {
    for (int i = 0; i < this->blockEntities.count; i++) {
      auto &blockEntity = this->blockEntities.list[i];
      if (blockEntity.position.x == pos.x && blockEntity.position.y == pos.y &&
          blockEntity.position.z == pos.z) {
        return true;
      }
    }
    return false;
  }

  void removeBlockEntity(const Vec3i &pos) {
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

  void setBlockStateId(const Vec3i &pos, int stateId) {
    auto section = this->getChunkSection(pos.y >> 4);
    section.setBlockStateId({pos.x, pos.y & 0xf, pos.z}, stateId);
  }

  void setBiomeId(const Vec3i &pos, int biomeId) {
    auto section = this->getBiomeSection(pos.y >> 4);
    section.setBiomeId({pos.x, pos.y & 0xf, pos.z}, biomeId);
  }

  int getLightIndex(const Vec3i &pos) {
    return pos.x + 8 * ((pos.y & 0xf) + 16 * pos.z);
  }

  void setBlockLight(const Vec3i &pos, int blockLight) {
    this->blockLights[co + (pos.y >> 4)].set(getLightIndex(pos), blockLight);
  }

  void setSkyLight(const Vec3i &pos, int skyLight) {
    this->skyLights[co + (pos.y >> 4)].set(getLightIndex(pos), skyLight);
  }

  void setBlockEntity(const Vec3i &pos, BlockEntity blockEntity) {
    for (int i = 0; i < this->blockEntities.count; i++) {
      if (this->blockEntities.list[i].position == pos) {
        this->blockEntities.list[i] = blockEntity;
        return;
      }
    }
    this->blockEntities.list = (BlockEntity *)reallocate(
        this->blockEntities.list,
        sizeof(BlockEntity) * (this->blockEntities.count),
        sizeof(BlockEntity) * (this->blockEntities.count + 1));
    assert(this->blockEntities.list != NULL);
    blockEntity.position = pos;
    this->blockEntities.list[this->blockEntities.count++] = blockEntity;
  }

  void writeNetworkSerializedTerrain(out u8 *&buffer, out int &bufferSize) {
    // This may seem expensive, but it's really cheap. We allocate the max size
    // possible on a CC on the stack (which is just moving stack pointer) then
    // we copy it over to the heap with the known size.
    // Max chunk size in Anvil/MCRegion format is 1MB/chunk
    // WASM has default stack size limit of 5MB, so no issue here.
    const int max_size = 1'000'000;

    u8 tempBuffer[max_size];
    BinaryStream stream(tempBuffer, max_size);

    for (int i = 0; i < this->numSections; i++) {
      this->sections[i].write(stream);
      this->biomes[i].write(stream);
    }

    buffer = (u8 *)malloc(stream.writePosition);
    bufferSize = stream.writePosition;
    stream.save(buffer);

    return;
  }

  void loadNetworkSerializedTerrain(BinaryStream &stream) {
    for (int i = 0; i < this->numSections; i++) {
      this->sections[i].read(stream);
      this->biomes[i].read(stream);
      printf("Done %d %d\n", i, stream.readPosition);
    }
    printf("Read net terrain %d / %d\n", stream.readPosition, stream.size);
    // stream.dumpRemaining();
  }

  void loadNetworkSerializedTerrain(u8 *buffer, int bufferSize) {
    BinaryStream stream(buffer, bufferSize);
    this->loadNetworkSerializedTerrain(stream);
  }

  void writeNetworkSerializedLights(BinaryStream &stream) {
    // ok, could be done with less code but Copilot came up with this and it's
    // actually more efficient :D
    int skyLights = 0;
    for (int i = 0; i < this->numSections + 1; i++) {
      auto mask = 1 << i;
      if (skyLightMask & mask) {
        skyLights++;
      }
    }
    stream.writeVarInt(skyLights);

    for (int i = 0; i < this->numSections + 1; i++) {
      auto mask = 1 << i;
      if (skyLightMask & mask) {
        stream.writeVarInt(2048);
        this->skyLights[i].write(stream);
      }
    }

    int blockLights = 0;
    for (int i = 0; i < this->numSections; i++) {
      auto mask = 1 << i;
      if (blockLightMask & mask) {
        blockLights++;
      }
    }
    stream.writeVarInt(blockLights);

    for (int i = 0; i < this->numSections; i++) {
      auto mask = 1 << i;
      if (blockLightMask & mask) {
        stream.writeVarInt(2048);
        this->blockLights[i].write(stream);
      }
    }
  }

  // https://wiki.vg/index.php?title=Protocol&oldid=17272#Chunk_Data_And_Update_Light
  void loadNetworkSerializedLights(u8 *skyLight, int skyLightLength,
                                   u8 *blockLight, int blockLightLength,
                                   u64 skyLightMask, u64 blockLightMask) {
    BinaryStream skyStream(skyLight, skyLightLength);
    BinaryStream blockStream(blockLight, blockLightLength);

    // toss the stupid extraneous light data because we don't store it
    this->blockLightMask = blockLightMask << 38 >> 38 >> 1;
    this->skyLightMask = skyLightMask << 38 >> 38 >> 1;

    int skyY = 0;
    int blockY = 0;
    for (int i = 0; i < this->numSections + 2; i++) {
      auto currentY = i - 1;
      auto sectionMask = (1 << i);
      // light data is sent for +/- 1 real world height, ignore those
      bool outOfBoundsWeTrack = i == 0 || i == (this->numSections + 1);

      if (skyLightMask & sectionMask) {
        if (outOfBoundsWeTrack) {
          skyStream.skip(2048);
        } else {
          auto skyLightForSection = skyLight[2048 * skyY++];
          this->skyLights[currentY].read(skyStream);
        }

        printf("reading skylight at y=%d\n", i - co);
      }
      if (blockLightMask & sectionMask) {
        if (outOfBoundsWeTrack) {
          blockStream.skip(2048);
        } else {
          auto blockLightForSection = blockLight[2048 * blockY++];
          this->blockLights[currentY].read(blockStream);
        }

        printf("reading blocklight at y=%d\n", i - co);
      }
    }
  }

  void writeSimpleHeightMap(BinaryStream &stream) {
    // TODO: actually we can't finish this until we implement the Registry
    // here... to know what ID an air block is auto bitsPerBlock =
    // log2ceil(this->numSections << 4); assert(bitsPerBlock == 9, "bitsPerBlock
    // is %d but expected 9",
    //        bitsPerBlock);
    // PalettedStorage<u64> storage(bitsPerBlock);
    // for (int x = 0; x < 16; x++) {
    //   for (int z = 0; z < 16; z++) {
    //     for (int y = maxY; y > minY; y++) {
    //       if (this->getBlockStateId({x, y, z}) != 0) {
    //         stream.writeVarInt(y);
    //         break;
    //       }
    //     }
    //   }
    // }
  }

  void writeChunkPacket(out u8 *&buffer, out int &bufferSize) {
    u8 *terrainData;
    int terrainLength = 0;
    this->writeNetworkSerializedTerrain(terrainData, terrainLength);

    const int max_size = 1'000'000;

    u8 tempBuffer[max_size];
    BinaryStream stream(tempBuffer, max_size);

    stream.writeIntBE(x);
    stream.writeIntBE(z);
    stream.writeByte(NBTTag::TAG_End);

    stream.writeVarInt(terrainLength);
    stream.write(terrainData, terrainLength);

    free(terrainData);

    stream.writeVarInt(this->blockEntities.count);
    for (int i = 0; i < this->blockEntities.count; i++) {
      auto &entity = this->blockEntities.list[i];
      stream.write((u8 *)entity.tag, entity.tagLength);
    }

    stream.writeByte(0);  // Trust edge lighting

    stream.writeVarInt(1);  // Sky light length
    stream.writeULongBE(skyLightMask);
    stream.writeVarInt(1);  // Block light length
    stream.writeULongBE(blockLightMask);
    // no idea what the point of "air masks" are
    stream.writeVarInt(1);  // Air mask length
    stream.writeULongBE(~skyLightMask);
    stream.writeVarInt(1);  // Block mask length
    stream.writeULongBE(~blockLightMask);

    this->writeNetworkSerializedLights(stream);

    buffer = (u8 *)malloc(stream.writePosition);
    bufferSize = stream.writePosition;
    stream.save(buffer);

    return;
  }

  static ChunkColumn *readChunkPacket(Registry *registry, u8 *buffer, int len) {
    BinaryStream stream(buffer, len);

    int x = stream.readIntBE();
    int z = stream.readIntBE();
    auto heightmaps = skipNBT(stream);
    if (!heightmaps) {
      // nbt reading error
      return nullptr;
    }

    ChunkColumn *chunk = new ChunkColumn(registry, x, z);

    // The extra zeros at the end are a pain to deal with... we have to skip
    // them here.
    auto chunkPayloadSize = stream.readVarInt();
    auto expectedNewPosition = stream.readPosition + chunkPayloadSize;
    chunk->loadNetworkSerializedTerrain(stream);
    // Extraneous zeros at the end of the payload need to be accounted for
    DEBUG_LOG("cc: skip %d bytes\n", expectedNewPosition - stream.readPosition);
    stream.readPosition = expectedNewPosition;

    // stream.dumpRemaining();

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
    if (skyLightMaskLen) {
      skyLightMask = stream.readLongBE();
    }

    auto blockLightMaskLen = stream.readVarInt();
    if (blockLightMaskLen) {
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
      auto skyLightLen = stream.readVarInt();
      assert(skyLightLen == 2048);
      stream.read(&skylight[2048 * i], skyLightLen);
    }
    auto blockLightLength = stream.readVarInt();
    for (int i = 0; i < blockLightLength; i++) {
      auto blockLightLen = stream.readVarInt();
      assert(blockLightLen == 2048);
      stream.read(&blocklight[2048 * i], blockLightLen);
    }

    chunk->loadNetworkSerializedLights(skylight, skyLightLength, blocklight,
                                       blockLightLength, skyLightMask,
                                       blockLightMask);

    // printf("At %d / %d\n", stream.readPosition, stream.size);
    // stream.dumpRemaining();
    return chunk;
  }
};
