//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// HashMap class.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__HashMap_H__
#define ACSVM__HashMap_H__

#include "List.hpp"
#include "Types.hpp"
#include "Vector.hpp"

#include <functional>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // HashMapGetKeyMem
   //
   // Used for HashMaps for which the Key is a member of T.
   //
   template<typename Key, typename T, Key const T::*KeyMem>
   struct HashMapGetKeyMem
   {
      static Key const &Get(T *obj) {return obj->*KeyMem;}
   };

   //
   // HashMapGetKeyObj
   //
   // Used for HashMaps for which the Key is a base class or the same as T.
   //
   template<typename Key, typename T>
   struct HashMapGetKeyObj
   {
      static Key const &Get(T *obj) {return *obj;}
   };

   //
   // HashMap
   //
   // Stores objects of type T that can be found by keys of type Key.
   //
   // GetKeyMem must be HashMapGetKeyMem or HashMapGetKeyObj.
   //
   // This class does not manage the lifetimes of the contained objects, and
   // objects must be unlinked by the unlink function before being destructed.
   // Although an exception is made to the latter if the clear function is
   // called before any other.
   //
   template<typename Key, typename T, typename GetKey, ListLink<T> T::*LinkMem,
      typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
   class HashMap
   {
   private:
      //
      // Iterator
      //
      template<typename Obj>
      class IteratorBase
      {
      public:
         //
         // operator ++
         //
         IteratorBase<Obj> &operator ++ ()
         {
            for(;;)
            {
               // Traverse to next link.
               link = link->next;

               // If it has an object, we are done.
               if(link->obj) break;

               // Otherwise, we are at the current chain's head. So increment
               // to the next chain. If at the last chain, we are done.
               if(++link == last) break;
            }

            return *this;
         }

         IteratorBase<Obj> operator ++ (int) {auto i = *this; ++*this; return i;}

         Obj &operator * () const {return *link->obj;}
         Obj *operator -> () const {return link->obj;}

         bool operator == (IteratorBase<Obj> const &iter) const
            {return iter.link == link;}
         bool operator != (IteratorBase<Obj> const &iter) const
            {return iter.link != link;}


         friend class HashMap;

      private:
         IteratorBase(ListLink<T> *link_, ListLink<T> *last_) :
            link{link_}, last{last_} {if(link != last) ++*this;}

         ListLink<T> *link, *last;
      };

   public:
      using const_iterator = IteratorBase<T const>;
      using iterator       = IteratorBase<T>;
      using size_type      = std::size_t;


      HashMap() : chainV{16}, objC{0}, growC{16} {}
      HashMap(size_type count, size_type growC_) :
         chainV{count}, objC{0}, growC{growC_} {}
      ~HashMap() {clear();}

      // begin
      iterator begin() {return {chainV.begin(), chainV.end()};}

      //
      // clear
      //
      void clear()
      {
         for(auto &chain : chainV)
         {
            while(auto obj = chain.next->obj)
               (obj->*LinkMem).unlink();
         }

         objC = 0;
      }

      // end
      iterator end() {return {chainV.end(), chainV.end()};}

      //
      // find
      //
      T *find(Key const &key)
      {
         for(auto itr = chainV[hasher(key) % chainV.size()].next; itr->obj; itr = itr->next)
         {
            if(equal(key, GetKey::Get(itr->obj)))
               return itr->obj;
         }

         return nullptr;
      }

      //
      // free
      //
      // Unlinks and deletes all contained objects.
      //
      void free()
      {
         for(auto &chain : chainV)
         {
            while(auto obj = chain.next->obj)
               (obj->*LinkMem).unlink(), delete obj;
         }

         objC = 0;
      }

      //
      // insert
      //
      void insert(T *obj)
      {
         if(objC >= chainV.size())
            resize(chainV.size() + chainV.size() / 2 + growC);

         ++objC;
         (obj->*LinkMem).insert(&chainV[hasher(GetKey::Get(obj)) % chainV.size()]);
      }

      //
      // resize
      //
      // Reallocates to count chains.
      //
      void resize(size_type count)
      {
         auto oldChainV = std::move(chainV);
         chainV.alloc(count);

         for(auto &chain : oldChainV)
         {
            while(auto obj = chain.next->obj)
               (obj->*LinkMem).relink(&chainV[hasher(GetKey::Get(obj)) % chainV.size()]);
         }
      }

      // size
      size_type size() const {return objC;}

      //
      // unlink
      //
      void unlink(T *obj)
      {
         --objC;
         (obj->*LinkMem).unlink();
      }

   private:
      Vector<ListLink<T>> chainV;
      Hash                hasher;
      KeyEqual            equal;

      size_type objC;
      size_type growC;
   };

   //
   // HashMapKeyMem
   //
   // Convenience typedef for HashMapGetKeyMem-based HashMaps.
   //
   template<typename Key, typename T, Key const T::*KeyMem,
      ListLink<T> T::*LinkMem, typename Hash = std::hash<Key>,
      typename KeyEqual = std::equal_to<Key>>
   using HashMapKeyMem = HashMap<Key, T, HashMapGetKeyMem<Key, T, KeyMem>,
      LinkMem, Hash, KeyEqual>;

   //
   // HashMapKeyObj
   //
   // Convenience typedef for HashMapGetKeyObj-based HashMaps.
   //
   template<typename Key, typename T, ListLink<T> T::*LinkMem,
      typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
   using HashMapKeyObj = HashMap<Key, T, HashMapGetKeyObj<Key, T>, LinkMem,
      Hash, KeyEqual>;

   //
   // HashMapElem
   //
   // Wraps a type with a key and link for use in HashMap.
   //
   template<typename Key, typename T>
   class HashMapElem
   {
   public:
      HashMapElem(Key const &key_, T const &val_) :
         key{key_}, val{val_}, link{this} {}

      Key key;
      T   val;

      ListLink<HashMapElem<Key, T>> link;
   };

   //
   // HashMapKeyExt
   //
   // Convenience typedef for HashMapElem-based HashMaps.
   //
   template<typename Key, typename T, typename Hash = std::hash<Key>,
      typename KeyEqual = std::equal_to<Key>>
   using HashMapKeyExt = HashMapKeyMem<Key, HashMapElem<Key, T>,
      &HashMapElem<Key, T>::key, &HashMapElem<Key, T>::link, Hash, KeyEqual>;
}

#endif//ACSVM__HashMap_H__

