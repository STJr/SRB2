//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// Stack class.
//
//-----------------------------------------------------------------------------

#ifndef ACSVM__Stack_H__
#define ACSVM__Stack_H__

#include <climits>
#include <new>
#include <utility>


//----------------------------------------------------------------------------|
// Types                                                                      |
//

namespace ACSVM
{
   //
   // Stack
   //
   // Stack container.
   //
   template<typename T>
   class Stack
   {
   public:
      Stack() : stack{nullptr}, stkEnd{nullptr}, stkPtr{nullptr} {}
      ~Stack() {clear(); ::operator delete(stack);}

      // operator []
      T &operator [] (std::size_t idx) {return *(stkPtr - idx);}

      // begin
      T       *begin()       {return stack;}
      T const *begin() const {return stack;}

      // clear
      void clear() {while(stkPtr != stack) (--stkPtr)->~T();}

      // drop
      void drop() {(--stkPtr)->~T();}
      void drop(std::size_t n) {while(n--) (--stkPtr)->~T();}

      // empty
      bool empty() const {return stkPtr == stack;}

      // end
      T       *end()       {return stkPtr;}
      T const *end() const {return stkPtr;}

      // push
      void push(T const &value) {new(stkPtr++) T(          value );}
      void push(T      &&value) {new(stkPtr++) T(std::move(value));}

      //
      // reserve
      //
      void reserve(std::size_t count)
      {
         if(static_cast<std::size_t>(stkEnd - stkPtr) >= count)
            return;

         // Save pointers as indexes.
         std::size_t idxEnd = stkEnd - stack;
         std::size_t idxPtr = stkPtr - stack;

         // Calculate new array size.
         if(SIZE_MAX / sizeof(T) - idxEnd < count * 2)
            throw std::bad_alloc();

         idxEnd += count * 2;

         // Allocate and initialize new array.
         T *stackNew = static_cast<T *>(::operator new(idxEnd * sizeof(T)));
         for(T *itrNew = stackNew, *itr = stack, *end = stkPtr; itr != end;)
         {
            new(itrNew++) T(std::move(*itr++));
            itr->~T();
         }

         // Free old array.
         ::operator delete(stack);

         // Restore pointers.
         stack  = stackNew;
         stkPtr = stack + idxPtr;
         stkEnd = stack + idxEnd;
      }

      // size
      std::size_t size() const {return stkPtr - stack;}

   private:
      T *stack;
      T *stkEnd;
      T *stkPtr;
   };
}

#endif//ACSVM__Stack_H__

