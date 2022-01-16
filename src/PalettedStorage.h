#pragma once
#include "BinaryStream.h"
#include "Mem.h"
#include "Types.h"

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
    this->blocksPerWord = FLOOR(wordBitSize / (float)bitsPerBlock);
    this->paddingPerWord = wordBitSize % bitsPerBlock;
    this->wordsCount = CEIL(capacity / (float)this->blocksPerWord);
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
    assert(index < this->wordsCount, "writing overflow");
    this->words[index] &= ~(this->mask << offset);
    this->words[index] |= (data & this->mask) << offset;
  }

  int get(int index) {
    int ix = FLOOR(index / this->blocksPerWord);
    int offset = (ix % blocksPerWord) * this->bitsPerBlock;
    return readBits(ix, offset);
  }

  void set(int index, int data) {
    int ix = FLOOR(index / this->blocksPerWord);
    int offset = (ix % blocksPerWord) * this->bitsPerBlock;
    writeBits(ix, offset, data);
  }

  void dump() {
    for (int i = 0; i < this->wordsCount; i++) {
      for (int j = 0; j < wordByteSize; j++) {
        printf("%02x", this->words[i] >> (j * 8));
      }
    }
    printf("\n");
  }

  ~PalettedStorage() { Deallocate(this->words); }
};
