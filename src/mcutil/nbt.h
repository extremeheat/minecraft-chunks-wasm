#pragma once
#include "../BinaryStream.h"
#include "../Types.h"

enum NBTTag : u8 {
  TAG_End = 0,
  TAG_Byte = 1,
  TAG_Short = 2,
  TAG_Int = 3,
  TAG_Long = 4,
  TAG_Float = 5,
  TAG_Double = 6,
  TAG_Byte_Array = 7,
  TAG_String = 8,
  TAG_List = 9,
  TAG_Compound = 10,
  TAG_Int_Array = 11,
  TAG_Long_Array = 12
};
const int MAX_TAG = 12;

void skipNBTPayload(BinaryStream &stream, NBTTag &type) {
  switch (type) {
    case TAG_End:
      break;
    case TAG_Byte:
      stream.skip(1);
      break;
    case TAG_Short:
      stream.skip(2);
      break;
    case TAG_Int:
      stream.skip(4);
      break;
    case TAG_Long:
      stream.skip(8);
      break;
    case TAG_Float:
      stream.skip(4);
      break;
    case TAG_Double:
      stream.skip(8);
      break;
    case TAG_Byte_Array:
      stream.skip(2);
      stream.skip(stream.readShortBE());
      break;
    case TAG_String:
      stream.skip(2);
      stream.skip(stream.readShortBE());
      break;
    case TAG_List: {
      auto listType = (NBTTag)stream.readByte();
      auto listLength = stream.readIntBE();
      for (int i = 0; i < listLength; i++) {
        skipNBTPayload(stream, listType);
      }
      break;
    }
    case TAG_Compound: {
      while (true) {
        auto tagType = (NBTTag)stream.readByte();
        if (tagType == TAG_End) {
          break;
        } else if (tagType <= MAX_TAG) {
          auto tagNameLen = stream.readShortBE();
          stream.skip(tagNameLen);
          skipNBTPayload(stream, tagType);
        } else {
          // assert(false, "Invalid tag type");
          throw "Invalid tag type";
        }
      }
      // stream.skip(2);
      // skipNBTPayload(stream, stream.readByte());
      break;
    }
    case TAG_Int_Array:
      stream.skip(stream.readIntBE() * 4);
      break;
    case TAG_Long_Array:
      stream.skip(stream.readIntBE() * 8);
      break;
    default:
      // assert(false, "Unknown tag type");
      throw "Unknown tag type";
  }
}

bool skipNBT(BinaryStream &stream) {
  auto tagType = (NBTTag)stream.readByte();
  if (tagType == TAG_End) {
    return false;
  } else if (tagType <= MAX_TAG) {
    auto tagNameLen = stream.readShortBE();
    stream.skip(tagNameLen);
    skipNBTPayload(stream, tagType);
    return true;
  } else {
    // assert(false, "Invalid tag type");
    throw "Invalid tag type";
  }
}

void getNBT(BinaryStream &stream, out char *buffer, out int len) {
  auto startingPosition = stream.readPosition;
  skipNBT(stream);
  auto endPosition = stream.readPosition;
  auto size = endPosition - startingPosition;
  stream.readPosition = startingPosition;
  stream.write(buffer, len);
}
