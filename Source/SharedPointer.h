#pragma once
#include "pch.h"

enum class ESPMode : uint8
{
    /** Forced to be not thread-safe. */
    NotThreadSafe = 0,

    /** Thread-safe, never spin locks, but slower */
    ThreadSafe = 1
};

template <ESPMode Mode>
class TReferenceControllerBase
{
    using RefCountType = std::conditional_t<Mode == ESPMode::ThreadSafe, std::atomic<int32>, int32>;

public:
    FORCEINLINE explicit TReferenceControllerBase() = default;

    // Number of shared references to this object.  When this count reaches zero, the associated object
    // will be destroyed (even if there are still weak references!), but not the reference controller.
    //
    // This starts at 1 because we create reference controllers via the construction of a TSharedPtr,
    // and that is the first reference.  There is no point in starting at 0 and then incrementing it.
    RefCountType SharedReferenceCount { 1 };

    // Number of weak references to this object.  If there are any shared references, that counts as one
    // weak reference too.  When this count reaches zero, the reference controller will be deleted.
    //
    // This starts at 1 because it represents the shared reference that we are also initializing
    // SharedReferenceCount with.
    RefCountType WeakReferenceCount { 1 };

    /** Destroys the object associated with this reference counter.  */
    virtual void DestroyObject() = 0;

    virtual ~TReferenceControllerBase()
    {
    }

    /** Returns the shared reference count */
    FORCEINLINE int32 GetSharedReferenceCount() const
    {
        if constexpr (Mode == ESPMode::ThreadSafe)
        {
            // A 'live' shared reference count is unstable by nature and so there's no benefit
            // to try and enforce memory ordering around the reading of it.
            //
            // This is equivalent to https://en.cppreference.com/w/cpp/memory/shared_ptr/use_count

            // This reference count may be accessed by multiple threads
            return SharedReferenceCount.load(std::memory_order_relaxed);
        }
        else
        {
            return SharedReferenceCount;
        }
    }

    /** Checks if there is exactly one reference left to the object. */
    FORCEINLINE bool IsUnique() const
    {
        if constexpr (Mode == ESPMode::ThreadSafe)
        {
            // This is equivalent to https://en.cppreference.com/w/cpp/memory/shared_ptr/unique,
            // however instead of deprecating it, we implement it with an acquire, which should
            // suit our use cases.

            // This reference count may be accessed by multiple threads
            return SharedReferenceCount.load(std::memory_order_acquire) == 1;
        }
        else
        {
            return SharedReferenceCount == 1;
        }
    }

    /** Adds a shared reference to this counter */
    FORCEINLINE void AddSharedReference()
    {
        if constexpr (Mode == ESPMode::ThreadSafe)
        {
            // Incrementing a reference count with relaxed ordering is always safe because no other action is taken
            // in response to the increment, so there's nothing to order with.

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
            ++SharedReferenceCount;
#else
            SharedReferenceCount.fetch_add(1, std::memory_order_relaxed);
#endif

            // If the transaction would abort, we need to undo adding the shared reference.
            ReleaseSharedReference();
        }
        else
        {
            ++SharedReferenceCount;
        }
    }

    /**
     * Adds a shared reference to this counter ONLY if there is already at least one reference
     *
     * @return  True if the shared reference was added successfully
     */
    bool ConditionallyAddSharedReference()
    {
        if constexpr (Mode == ESPMode::ThreadSafe)
        {
            bool bSucceeded = false;

            // See AddSharedReference for the same reasons that std::memory_order_relaxed is used in this function.

            // Peek at the current shared reference count.  Remember, this value may be updated by
            // multiple threads.
            int32 OriginalCount = SharedReferenceCount.load(std::memory_order_relaxed);

            for (;;)
            {
                if (OriginalCount == 0)
                {
                    // Never add a shared reference if the pointer has already expired
                    bSucceeded = false;
                    break;
                }

                // Attempt to increment the reference count.
                //
                // We need to make sure that we never revive a counter that has already expired, so if the
                // actual value what we expected (because it was touched by another thread), then we'll try
                // again.  Note that only in very unusual cases will this actually have to loop.
                //
                // We do a weak read here because we require a loop and this is the recommendation:
                //
                // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
                //
                // > When a compare-and-exchange is in a loop, the weak version will yield better performance on some platforms.
                // > When a weak compare-and-exchange would require a loop and a strong one would not, the strong one is preferable
                if (SharedReferenceCount.compare_exchange_weak(OriginalCount, OriginalCount + 1, std::memory_order_relaxed))
                {
                    bSucceeded = true;
                    break;
                }
            }

            // If we succeedd in taking a shared reference count, we need to undo that on an abort.
            if (bSucceeded)
            {
                ReleaseSharedReference();
            }

            return bSucceeded;
        }
        else
        {
            if (SharedReferenceCount == 0)
            {
                // Never add a shared reference if the pointer has already expired
                return false;
            }

            ++SharedReferenceCount;
            return true;
        }
    }

    /** Releases a shared reference to this counter */
    FORCEINLINE void ReleaseSharedReference()
    {
        if constexpr (Mode == ESPMode::ThreadSafe)
        {
            // std::memory_order_acq_rel is used here so that, if we do end up executing the destructor, it's not possible
            // for side effects from executing the destructor end up being visible before we've determined that the shared
            // reference count is actually zero.

            int32 OldSharedCount = SharedReferenceCount.fetch_sub(1, std::memory_order_acq_rel);
            if (OldSharedCount == 1)
            {
                // Last shared reference was released!  Destroy the referenced object.
                DestroyObject();

                // No more shared referencers, so decrement the weak reference count by one.  When the weak
                // reference count reaches zero, this object will be deleted.
                ReleaseWeakReference();
            }
        }
        else
        {
            if (--SharedReferenceCount == 0)
            {
                // Last shared reference was released!  Destroy the referenced object.
                DestroyObject();

                // No more shared referencers, so decrement the weak reference count by one.  When the weak
                // reference count reaches zero, this object will be deleted.
                ReleaseWeakReference();
            }
        }
    }

    /** Adds a weak reference to this counter */
    FORCEINLINE void AddWeakReference()
    {
        if constexpr (Mode == ESPMode::ThreadSafe)
        {
            // See AddSharedReference for the same reasons that std::memory_order_relaxed is used in this function.

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
            // We do a regular SC increment here because it maps to an _InterlockedIncrement (lock inc).
            // The codegen for a relaxed fetch_add is actually much worse under MSVC (lock xadd).
            ++WeakReferenceCount;
#else
            WeakReferenceCount.fetch_add(1, std::memory_order_relaxed);
#endif
        }
        else
        {
            ++WeakReferenceCount;
        }
    }

    /** Releases a weak reference to this counter */
    void ReleaseWeakReference()
    {
        if constexpr (Mode == ESPMode::ThreadSafe)
        {
            // See ReleaseSharedReference for the same reasons that std::memory_order_acq_rel is used in this function.

            int32 OldWeakCount = WeakReferenceCount.fetch_sub(1, std::memory_order_acq_rel);
            if (OldWeakCount == 1)
            {
                // Disable this if running clang's static analyzer. Passing shared pointers
                // and references to functions it cannot reason about, produces false
                // positives about use-after-free in the TSharedPtr/TSharedRef destructors.
#if !defined(__clang_analyzer__)
                // No more references to this reference count.  Destroy it!
                delete this;
#endif
            }
        }
        else
        {
            checkSlow(WeakReferenceCount > 0);

            if (--WeakReferenceCount == 0)
            {
                // No more references to this reference count.  Destroy it!
#if !defined(__clang_analyzer__)
                delete this;
#endif
            }
        }
    }

    // Non-copyable
    TReferenceControllerBase(const TReferenceControllerBase&) = delete;
    TReferenceControllerBase& operator=(const TReferenceControllerBase&) = delete;
};

template <ESPMode Mode>
class FSharedReferencer
{
public:
    /** Constructor for an empty shared referencer object */
    FORCEINLINE FSharedReferencer()
        : ReferenceController(nullptr)
    {
    }

    /** Constructor that counts a single reference to the specified object */
    inline explicit FSharedReferencer(TReferenceControllerBase<Mode>* InReferenceController)
        : ReferenceController(InReferenceController)
    {
    }

    /** Copy constructor creates a new reference to the existing object */
    FORCEINLINE FSharedReferencer(FSharedReferencer const& InSharedReference)
        : ReferenceController(InSharedReference.ReferenceController)
    {
        // If the incoming reference had an object associated with it, then go ahead and increment the
        // shared reference count
        if (ReferenceController != nullptr)
        {
            ReferenceController->AddSharedReference();
        }
    }

    /** Move constructor creates no new references */
    FORCEINLINE FSharedReferencer(FSharedReferencer&& InSharedReference)
        : ReferenceController(InSharedReference.ReferenceController)
    {
        InSharedReference.ReferenceController = nullptr;
    }

    /** Destructor. */
    FORCEINLINE ~FSharedReferencer()
    {
        if (ReferenceController != nullptr)
        {
            // Tell the reference counter object that we're no longer referencing the object with
            // this shared pointer
            ReferenceController->ReleaseSharedReference();
        }
    }

    /** Assignment operator adds a reference to the assigned object.  If this counter was previously
        referencing an object, that reference will be released. */
    inline FSharedReferencer& operator=(FSharedReferencer const& InSharedReference)
    {
        // Make sure we're not be reassigned to ourself!
        auto NewReferenceController = InSharedReference.ReferenceController;
        if (NewReferenceController != ReferenceController)
        {
            // First, add a shared reference to the new object
            if (NewReferenceController != nullptr)
            {
                NewReferenceController->AddSharedReference();
            }

            // Release shared reference to the old object
            if (ReferenceController != nullptr)
            {
                ReferenceController->ReleaseSharedReference();
            }

            // Assume ownership of the assigned reference counter
            ReferenceController = NewReferenceController;
        }

        return *this;
    }

    /** Move assignment operator adds no references to the assigned object.  If this counter was previously
        referencing an object, that reference will be released. */
    inline FSharedReferencer& operator=(FSharedReferencer&& InSharedReference)
    {
        // Make sure we're not be reassigned to ourself!
        auto NewReferenceController = InSharedReference.ReferenceController;
        auto OldReferenceController = ReferenceController;
        if (NewReferenceController != OldReferenceController)
        {
            // Assume ownership of the assigned reference counter
            InSharedReference.ReferenceController = nullptr;
            ReferenceController = NewReferenceController;

            // Release shared reference to the old object
            if (OldReferenceController != nullptr)
            {
                OldReferenceController->ReleaseSharedReference();
            }
        }

        return *this;
    }

    /**
     * Tests to see whether or not this shared counter contains a valid reference
     *
     * @return  True if reference is valid
     */
    FORCEINLINE const bool IsValid() const
    {
        return ReferenceController != nullptr;
    }

    /**
     * Returns the number of shared references to this object (including this reference.)
     *
     * @return  Number of shared references to the object (including this reference.)
     */
    FORCEINLINE const int32 GetSharedReferenceCount() const
    {
        return ReferenceController != nullptr ? ReferenceController->GetSharedReferenceCount() : 0;
    }

    /**
     * Returns true if this is the only shared reference to this object.  Note that there may be
     * outstanding weak references left.
     *
     * @return  True if there is only one shared reference to the object, and this is it!
     */
    FORCEINLINE const bool IsUnique() const
    {
        return ReferenceController != nullptr && ReferenceController->IsUnique();
    }

private:
    // Expose access to ReferenceController to FWeakReferencer
    template <ESPMode OtherMode>
    friend class FWeakReferencer;

private:
    /** Pointer to the reference controller for the object a shared reference/pointer is referencing */
    TReferenceControllerBase<Mode>* ReferenceController;
};

template <class ObjectType, ESPMode InMode>
class TSharedPtr
{
public:
    using ElementType = ObjectType;
    static constexpr ESPMode Mode = InMode;

    FORCEINLINE ObjectType* Get() const
    {
        return Object;
    }

    /**
     * Checks to see if this shared pointer is actually pointing to an object
     *
     * @return  True if the shared pointer is valid and can be dereferenced
     */
    FORCEINLINE explicit operator bool() const
    {
        return Object != nullptr;
    }

    /**
     * Checks to see if this shared pointer is actually pointing to an object
     *
     * @return  True if the shared pointer is valid and can be dereferenced
     */
    FORCEINLINE const bool IsValid() const
    {
        return Object != nullptr;
    }

    /**
     * Dereference operator returns a reference to the object this shared pointer points to
     *
     * @return  Reference to the object
     */
    FORCEINLINE decltype(auto) operator*() const
    {
        return *Object;
    }

    FORCEINLINE ObjectType* operator->()
    {
        return Object;
    }

    /** The object we're holding a reference to.  Can be nullptr. */
    ObjectType* Object;

    /** Interface to the reference counter for this object.  Note that the actual reference
            controller object is shared by all shared and weak pointers that refer to the object */
    FSharedReferencer<Mode> SharedReferenceCount;
};

template <class ObjectType, ESPMode InMode>
class TSharedRef
{
public:
    using ElementType = ObjectType;
    static constexpr ESPMode Mode = InMode;

    FORCEINLINE ObjectType& Get() const
    {
        return *Object;
    }

    /**
     * Dereference operator returns a reference to the object this shared pointer points to
     *
     * @return  Reference to the object
     */
    FORCEINLINE ObjectType& operator*() const
    {
        return *Object;
    }

    /**
     * Arrow operator returns a pointer to this shared reference's object
     *
     * @return  Returns a pointer to the object referenced by this shared reference
     */
    FORCEINLINE ObjectType* operator->() const
    {
        return Object;
    }

    /** The object we're holding a reference to.  Can be nullptr. */
    ObjectType* Object;

    /** Interface to the reference counter for this object.  Note that the actual reference
            controller object is shared by all shared and weak pointers that refer to the object */
    FSharedReferencer<Mode> SharedReferenceCount;
};
