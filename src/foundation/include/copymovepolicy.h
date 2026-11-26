#pragma once

#include <type_traits>

namespace vwz
{
    namespace core
    {
        // allowMove = false : Context object. Not copyable and not movable.
        // allowMove = true  : Resource object. Not copyable. Must support noexcept move operations.
        template <class Derived, bool allowMove = true>
        class CopyMovePolicy
        {
        protected:
            constexpr CopyMovePolicy() = default;
            ~CopyMovePolicy() noexcept
            {
                // Both context and resource objects must not be copyable.
                static_assert
                (
                    !std::is_copy_constructible<Derived>::value,
                    "CopyMovePolicy: Derived must not be copy constructible"
                );

                static_assert
                (
                    !std::is_copy_assignable<Derived>::value,
                    "CopyMovePolicy: Derived must not be copy assignable"
                );

                if constexpr (allowMove)
                {
                    // Resource object: must support noexcept move operations.
                    static_assert
                    (
                        std::is_nothrow_move_constructible<Derived>::value,
                        "CopyMovePolicy: resource class must support noexcept move construction"
                    );

                    static_assert
                    (
                        std::is_nothrow_move_assignable<Derived>::value,
                        "CopyMovePolicy: resource class must support noexcept move assignment"
                    );
                }
                else
                {
                    static_assert
                    (
                        !std::is_move_constructible<Derived>::value,
                        "CopyMovePolicy: context class must not be move constructible"
                    );

                    static_assert
                    (
                        !std::is_move_assignable<Derived>::value,
                        "CopyMovePolicy: context class must not be move assignable"
                    );
                }
            }

            CopyMovePolicy(const CopyMovePolicy&) = delete;
            CopyMovePolicy& operator=(const CopyMovePolicy&) = delete;

            CopyMovePolicy(CopyMovePolicy&&) = delete;
            CopyMovePolicy& operator=(CopyMovePolicy&&) = delete;
        };
    }
}