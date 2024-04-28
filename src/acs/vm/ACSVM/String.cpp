//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// String classes.
//
//-----------------------------------------------------------------------------

#include "String.hpp"

#include "BinaryIO.hpp"
#include "HashMap.hpp"

#include <new>
#include <vector>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // StringTable::PrivData
   //
   struct StringTable::PrivData
   {
      std::vector<Word> freeIdx;

      HashMapKeyObj<StringData, String, &String::link> stringByData{64, 64};
      std::vector<String *>                            stringByIdx;
   };
}


//----------------------------------------------------------------------------|
// Extern Functions                                                           |
//

namespace ACSVM
{
   //
   // String constructor
   //
   String::String(StringData const &data, Word idx_) :
      StringData{data}, lock{0}, idx{idx_}, len0(std::strlen(str)), link{this}
   {
   }

   //
   // String destructor
   //
   String::~String()
   {
   }

   //
   // String::Delete
   //
   void String::Delete(String *str)
   {
      str->~String();
      operator delete(str);
   }

   //
   // String::New
   //
   String *String::New(StringData const &data, Word idx)
   {
      String *str = static_cast<String *>(operator new(sizeof(String) + data.len + 1));
      char   *buf = reinterpret_cast<char *>(str + 1);

      memcpy(buf, data.str, data.len);
      buf[data.len] = '\0';

      return new(str) String{{buf, data.len, data.hash}, idx};
   }

   //
   // String::Read
   //
   String *String::Read(std::istream &in, Word idx)
   {
      std::size_t len = ReadVLN<std::size_t>(in);

      String *str = static_cast<String *>(operator new(sizeof(String) + len + 1));
      char   *buf = reinterpret_cast<char *>(str + 1);

      in.read(buf, len);
      buf[len] = '\0';

      return new(str) String{{buf, len, StrHash(buf, len)}, idx};
   }

   //
   // String::Write
   //
   void String::Write(std::ostream &out, String *in)
   {
      WriteVLN(out, in->len);
      out.write(in->str, in->len);
   }

   //
   // StringTable constructor
   //
   StringTable::StringTable() :
      strV{nullptr},
      strC{0},

      strNone{String::New({"", 0, 0}, 0)},

      pd{new PrivData}
   {
   }

   //
   // StringTable move constructor
   //
   StringTable::StringTable(StringTable &&table) :
      strV{table.strV},
      strC{table.strC},

      strNone{table.strNone},

      pd{table.pd}
   {
      table.strV = nullptr;
      table.strC = 0;

      table.strNone = nullptr;

      table.pd = nullptr;
   }

   //
   // StringTable destructor
   //
   StringTable::~StringTable()
   {
      if(!pd) return;

      clear();

      delete pd;

      String::Delete(strNone);
   }

   //
   // StringTable::operator [StringData]
   //
   String &StringTable::operator [] (StringData const &data)
   {
      if(auto str = pd->stringByData.find(data)) return *str;

      Word idx;
      if(pd->freeIdx.empty())
      {
         // Index has to fit within Word size.
         // If size_t has an equal or lesser max, then the check is redundant,
         // and some compilers warn about that kind of tautological comparison.
         #if SIZE_MAX > UINT32_MAX
         if(pd->stringByIdx.size() > UINT32_MAX)
            throw std::bad_alloc();
         #endif

         idx = pd->stringByIdx.size();
         pd->stringByIdx.emplace_back(strNone);
         strV = pd->stringByIdx.data();
         strC = pd->stringByIdx.size();
      }
      else
      {
         idx = pd->freeIdx.back();
         pd->freeIdx.pop_back();
      }

      String *str = String::New(data, idx);
      pd->stringByIdx[idx] = str;
      pd->stringByData.insert(str);
      return *str;
   }

   //
   // StringTable::clear
   //
   void StringTable::clear()
   {
      for(auto &str : pd->stringByIdx)
      {
         if(str != strNone)
            String::Delete(str);
      }

      pd->freeIdx.clear();
      pd->stringByData.clear();
      pd->stringByIdx.clear();

      strV = nullptr;
      strC = 0;
   }

   //
   // StringTable::collectBegin
   //
   void StringTable::collectBegin()
   {
      for(auto &str : pd->stringByData)
         str.ref = false;
   }

   //
   // StringTable.collectEnd
   //
   void StringTable::collectEnd()
   {
      for(auto itr = pd->stringByData.begin(), end = pd->stringByData.end(); itr != end;)
      {
         if(!itr->ref && !itr->lock)
         {
            String &str = *itr++;
            pd->stringByIdx[str.idx] = strNone;
            pd->freeIdx.push_back(str.idx);
            pd->stringByData.unlink(&str);
            String::Delete(&str);
         }
         else
            ++itr;
      }
   }

   //
   // StringTable::loadState
   //
   void StringTable::loadState(std::istream &in)
   {
      if(pd)
      {
         clear();
      }
      else
      {
         pd      = new PrivData;
         strNone = String::New({"", 0, 0}, 0);
      }

      auto count = ReadVLN<std::size_t>(in);

      pd->stringByIdx.resize(count);
      strV = pd->stringByIdx.data();
      strC = pd->stringByIdx.size();

      for(std::size_t idx = 0; idx != count; ++idx)
      {
         if(in.get())
         {
            String *str = String::Read(in, idx);
            str->lock = ReadVLN<std::size_t>(in);
            pd->stringByIdx[idx] = str;
            pd->stringByData.insert(str);
         }
         else
         {
            pd->stringByIdx[idx] = strNone;
            pd->freeIdx.emplace_back(idx);
         }
      }
   }

   //
   // StringTable::saveState
   //
   void StringTable::saveState(std::ostream &out) const
   {
      WriteVLN(out, pd->stringByIdx.size());

      for(String *&str : pd->stringByIdx)
      {
         if(str != strNone)
         {
            out << '\1';

            String::Write(out, str);
            WriteVLN(out, str->lock);
         }
         else
            out << '\0';
      }
   }

   //
   // StringTable::size
   //
   std::size_t StringTable::size() const
   {
      return pd->stringByData.size();
   }

   //
   // StrDup
   //
   std::unique_ptr<char[]> StrDup(char const *str)
   {
      return StrDup(str, std::strlen(str));
   }

   //
   // StrDup
   //
   std::unique_ptr<char[]> StrDup(char const *str, std::size_t len)
   {
      std::unique_ptr<char[]> dup{new char[len + 1]};
      std::memcpy(dup.get(), str, len);
      dup[len] = '\0';

      return dup;
   }

   //
   // StrHash
   //
   std::size_t StrHash(char const *str)
   {
      std::size_t hash = 0;

      if(str) while(*str)
         hash = hash * 5 + static_cast<unsigned char>(*str++);

      return hash;
   }

   //
   // StrHash
   //
   std::size_t StrHash(char const *str, std::size_t len)
   {
      std::size_t hash = 0;

      while(len--)
         hash = hash * 5 + static_cast<unsigned char>(*str++);

      return hash;
   }
}

// EOF

