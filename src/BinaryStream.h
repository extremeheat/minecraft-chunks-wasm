#pragma once
#include "Types.h"

class BinaryStream {
 public:
  unsigned char *data;
  int readPosition = 0;
  int writePosition = 0;
  BinaryStream(int size) { this->data = Allocate<unsigned char>(size); }

  BinaryStream(void *data, int size) { this->data = (unsigned char *)data; }

  void read(void *data, int size) {
    memcpy(data, this->data + this->readPosition, size);
    this->readPosition += size;
  }

  void write(void *data, int size) {
    printf("write %d bytes\n", size);
    memcpy(this->data + this->writePosition, data, size);
    this->writePosition += size;
  }

  i64 readLongLE() {
    long long result = 0;
    for (int i = 0; i < 8; i++) {
      result |= (long long)this->readByte() << (i * 8);
    }
    return result;
  }

  i64 readLongBE() {
    long long result = 0;
    for (int i = 0; i < 8; i++) {
      result |= (long long)this->readByte() << ((7 - i) * 8);
    }
    return result;
  }

  u64 readULongLE() {
    u64 result = 0;
    for (int i = 0; i < 8; i++) {
      result |= (u64)this->readByte() << (i * 8);
    }
    return result;
  }

  u64 readULongBE() {
    unsigned long long result = 0;
    for (int i = 0; i < 8; i++) {
      result |= (u64)this->readByte() << ((7 - i) * 8);
    }
    return result;
  }

  int readIntLE() {
    int result = 0;
    for (int i = 0; i < 4; i++) {
      result |= this->readByte() << (i * 8);
    }
    return result;
  }

  int readIntBE() {
    int result = 0;
    for (int i = 0; i < 4; i++) {
      result |= this->readByte() << ((3 - i) * 8);
    }
    return result;
  }

  u32 readUIntLE() {
    u32 result = 0;
    for (int i = 0; i < 4; i++) {
      result |= this->readByte() << (i * 8);
    }
    return result;
  }

  short readShortLE() {
    short result = 0;
    for (int i = 0; i < 2; i++) {
      result |= this->readByte() << (i * 8);
    }
    return result;
  }

  short readShortBE() {
    short result = 0;
    for (int i = 0; i < 2; i++) {
      result |= this->readByte() << ((1 - i) * 8);
    }
    return result;
  }

  u16 readUShortLE() {
    u16 result = 0;
    for (int i = 0; i < 2; i++) {
      result |= this->readByte() << (i * 8);
    }
    return result;
  }

  u16 readUShortBE() {
    u16 result = 0;
    for (int i = 0; i < 2; i++) {
      result |= this->readByte() << ((1 - i) * 8);
    }
    return result;
  }

  unsigned char readByte() { return this->data[this->readPosition++]; }

  i32 readVarInt() {
    int result = 0;
    int shift = 0;
    while (true) {
      unsigned char byte = this->readByte();
      result |= (byte & 0x7f) << shift;
      if ((byte & 0x80) == 0) {
        break;
      }
      shift += 7;
    }
    return result;
  }

  u32 readUVarInt() {
    u32 result = 0;
    int shift = 0;
    while (true) {
      unsigned char byte = this->readByte();
      result |= (byte & 0x7f) << shift;
      if ((byte & 0x80) == 0) {
        break;
      }
      shift += 7;
    }
    return result;
  }

  i64 readVarLong() {
    long long result = 0;
    int shift = 0;
    while (true) {
      unsigned char byte = this->readByte();
      result |= (byte & 0x7f) << shift;
      if ((byte & 0x80) == 0) {
        break;
      }
      shift += 7;
    }
    return result;
  }

  u64 readUVarLong() {
    u64 result = 0;
    int shift = 0;
    while (true) {
      unsigned char byte = this->readByte();
      result |= (byte & 0x7f) << shift;
      if ((byte & 0x80) == 0) {
        break;
      }
      shift += 7;
    }
    return result;
  }

  void writeLongLE(i64 value) {
    for (int i = 0; i < 8; i++) {
      this->writeByte((value >> (i * 8)) & 0xff);
    }
  }

  void writeLongBE(i64 value) {
    for (int i = 0; i < 8; i++) {
      this->writeByte((value >> ((7 - i) * 8)) & 0xff);
    }
  }

  void writeULongLE(u64 value) {
    for (int i = 0; i < 8; i++) {
      this->writeByte((value >> (i * 8)) & 0xff);
    }
  }

  void writeULongBE(u64 value) {
    for (int i = 0; i < 8; i++) {
      this->writeByte((value >> ((7 - i) * 8)) & 0xff);
    }
  }

  void writeIntLE(i32 value) {
    for (int i = 0; i < 4; i++) {
      this->writeByte((value >> (i * 8)) & 0xff);
    }
  }

  void writeIntBE(i32 value) {
    for (int i = 0; i < 4; i++) {
      this->writeByte((value >> ((3 - i) * 8)) & 0xff);
    }
  }

  void writeUIntLE(u32 value) {
    for (int i = 0; i < 4; i++) {
      this->writeByte((value >> (i * 8)) & 0xff);
    }
  }

  void writeShortLE(short value) {
    for (int i = 0; i < 2; i++) {
      this->writeByte((value >> (i * 8)) & 0xff);
    }
  }

  void writeShortBE(short value) {
    for (int i = 0; i < 2; i++) {
      this->writeByte((value >> ((1 - i) * 8)) & 0xff);
    }
  }

  void writeUShortLE(u16 value) {
    for (int i = 0; i < 2; i++) {
      this->writeByte((value >> (i * 8)) & 0xff);
    }
  }

  void writeUShortBE(u16 value) {
    for (int i = 0; i < 2; i++) {
      this->writeByte((value >> ((1 - i) * 8)) & 0xff);
    }
  }

  void writeByte(unsigned char value) {
    this->data[this->writePosition++] = value;
  }

  void writeVarInt(i32 value) {
    while (true) {
      unsigned char byte = value & 0x7f;
      value >>= 7;
      if (value != 0) {
        byte |= 0x80;
      }
      this->writeByte(byte);
      if (value == 0) {
        break;
      }
    }
  }

  void writeUVarInt(u32 value) {
    while (true) {
      unsigned char byte = value & 0x7f;
      value >>= 7;
      if (value != 0) {
        byte |= 0x80;
      }
      this->writeByte(byte);
      if (value == 0) {
        break;
      }
    }
  }

  void writeVarLong(i64 value) {
    while (true) {
      unsigned char byte = value & 0x7f;
      value >>= 7;
      if (value != 0) {
        byte |= 0x80;
      }
      this->writeByte(byte);
      if (value == 0) {
        break;
      }
    }
  }

  void writeUVarLong(u64 value) {
    while (true) {
      unsigned char byte = value & 0x7f;
      value >>= 7;
      if (value != 0) {
        byte |= 0x80;
      }
      this->writeByte(byte);
      if (value == 0) {
        break;
      }
    }
  }

  // void writeString(std::string value) {
  //   this->writeVarInt(value.length());
  //   for (int i = 0; i < value.length(); i++) {
  //     this->writeByte(value[i]);
  //   }
  // }

  // std::string readString() {
  //   int length = this->readVarInt();
  //   std::string result = "";
  //   for (int i = 0; i < length; i++) {
  //     result += this->readByte();
  //   }
  //   return result;
  // }

  ~BinaryStream() { Deallocate(this->data); }
};
