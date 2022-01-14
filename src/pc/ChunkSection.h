#pragma once
#include "../PalettedStorage.h"

struct ChunkSection {
  short blocks[4096];
  short palette[4096];
  // if this is zero this section does not exist
  int paletteLength = 0;

  inline bool isEmpty() {
    return paletteLength == 0;
  }

  inline int getIndex(const Vec3 &pos) {
    return (pos.y & 0xf) << 8 | (pos.z & 0xf) << 4 | (pos.x & 0xf);
  }

  void setBlockStateId(const Vec3 &pos, int stateId) {
    blocks[getIndex(pos)] = stateId;
  }

  int getBlockStateId(const Vec3 &pos) {
    return blocks[getIndex(pos)];
  }

  void read(BinaryStream &stream) {
    u16 occupiedBlocks = stream.readShortBE();
    u8 bitsPerBlock = stream.readByte();

    if (!bitsPerBlock) {
      this->paletteLength = 1;
      this->palette[0] = stream.readVarInt();
      assert(stream.readByte() == 0, "Expected to read 0 length data for 1 length palette");
      return;
    }

    this->paletteLength = stream.readVarInt();
    for (int i = 0; i < this->paletteLength; i++) {
      this->palette[i] = stream.readVarInt();
    }

    auto storage = PalettedStorage(bitsPerBlock);
    storage.read(stream);

    for (int i = 0; i < 4096; i++) {
      blocks[i] = storage.getAt(i & 0xf, (i >> 8) & 0xf, (i >> 4) & 0xf);
    }
  }

  void write(BinaryStream &stream) {
    auto bitsPerBlock = 32 - __builtin_clz(this->paletteLength - 1);

    if (!bitsPerBlock) {
      stream.writeShortBE(0); // occupied blocks
      stream.writeByte(0); // bits per block
      stream.writeVarInt(this->palette[0]); // palette
      stream.writeByte(0); // palette length
      return;
    }

    auto storage = PalettedStorage(bitsPerBlock);

    for (int i = 0; i < 4096; i++) {
      storage.setAt(i & 0xf, (i >> 8) & 0xf, (i >> 4) & 0xf, blocks[i]);
    }

    storage.write(stream);
  }
};

class BiomeSection : public ChunkSection {
public:
  void setBiomeId(const Vec3 &pos, int biome) {
    blocks[getIndex(pos)] = biome;
  }

  int getBiomeId(const Vec3 &pos) {
    return blocks[getIndex(pos)];
  }
};