#pragma once
#include "Types.h"
#include "Mem.h"
#include "BinaryStream.h"

template <typename Word = unsigned int>
class PalettedStorage {
 public:
  const int wordByteSize = sizeof(Word);
  const int wordBitSize = wordByteSize * 8;
  int bitsPerBlock = 0;
  int blocksPerWord = 0;
  int paddingPerWord = 0;
  int wordsCount = 0;
  int mask = 0;
  int byteSize = 0;
  Word *words = nullptr;

  PalettedStorage() {}

  PalettedStorage(int bitsPerBlock, int capacity = 4096) {
    init(bitsPerBlock, capacity);
  }

  void init(int bitsPerBlock, int capacity = 4096) {
    this->bitsPerBlock = bitsPerBlock;
    this->blocksPerWord = FLOOR(wordBitSize / bitsPerBlock);
    this->paddingPerWord = wordBitSize % bitsPerBlock;
    this->wordsCount = CEIL(capacity / this->blocksPerWord);
    this->byteSize = this->wordsCount * wordByteSize;
    this->mask = (1 << bitsPerBlock) - 1;

    this->words = Allocate<Word>(this->wordsCount);
  }

  void read(BinaryStream &stream) { stream.read(this->words, this->byteSize); }

  void write(BinaryStream &stream) {
    stream.write(this->words, this->byteSize);
  }

  int readBits(int index, int offset) {
    return (this->words[index] >> offset) & this->mask;
  }

  void writeBits(int index, int offset, int data) {
    this->words[index] &= ~(this->mask << offset);
    this->words[index] |= (data & this->mask) << offset;
  }

  void getIndex(byte x, byte y, byte z, int &index, int &offset) {
    x &= 0xf;
    y &= 0xf;
    z &= 0xf;

    index = FLOOR(((x << 8) | (z << 4) | y) / blocksPerWord);
    offset = (((x << 8) | (z << 4) | y) % blocksPerWord) * this->bitsPerBlock;
  }

  int getAt(byte x, byte y, byte z) {
    int index, offset;
    getIndex(x, y, z, index, offset);
    return readBits(index, offset);
  }

  void setAt(byte x, byte y, byte z, int data) {
    int index, offset;
    getIndex(x, y, z, index, offset);
    writeBits(index, offset, data);
  }

  void dump() {
    for (int i = 0; i < this->wordsCount; i++) {
      // printf("%d: %d\n", i, this->words[i]);
      // print hex
      for (int j = 0; j < wordByteSize; j++) {
        printf("%02x", this->words[i] >> (j * 8));
      }
    }
    printf("\n");
  }

  void dumpAll() {
    for (int x = 0; x < 16; x++) {
      for (int y = 0; y < 16; y++) {
        for (int z = 0; z < 16; z++) {
          printf("x: %d y: %d z: %d data: %d\n", x, y, z, getAt(x, y, z));
        }
      }
    }
  }

  ~PalettedStorage() { Deallocate(this->words); }
};

inline void test_palletedStorage() {
  PalettedStorage<> storage(31);
  storage.setAt(1, 0, 0, 0);
  storage.setAt(0, 0, 1, 1);
  storage.setAt(0, 0, 2, 2);
  // storage.dumpAll();
  assert(storage.getAt(1, 0, 0) == 0, "1");
  assert(storage.getAt(0, 0, 1) == 1, "2");
  assert(storage.getAt(0, 0, 2) == 2);

  BinaryStream stream(storage.byteSize);
  storage.write(stream);

  PalettedStorage<> storage2(31);
  storage2.read(stream);
  // storage2.dumpAll();
  assert(storage2.getAt(1, 0, 0) == 0);
  assert(storage2.getAt(0, 0, 1) == 1);
  assert(storage2.getAt(0, 0, 2) == 2);
}