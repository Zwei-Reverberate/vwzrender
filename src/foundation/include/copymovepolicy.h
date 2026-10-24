#pragma once

namespace vwz
{
    namespace core
    {
        // It is recommended to use private inheritance
        template <bool allowMove = true>
        class  CopyMovePolicy
        {
        protected:
            constexpr  CopyMovePolicy() = default;
            ~ CopyMovePolicy() = default;

             CopyMovePolicy(const  CopyMovePolicy&) = delete;
             CopyMovePolicy& operator=(const  CopyMovePolicy&) = delete;

             CopyMovePolicy( CopyMovePolicy&&) noexcept requires(allowMove) = default;
             CopyMovePolicy& operator=( CopyMovePolicy&&) noexcept requires(allowMove) = default;

             CopyMovePolicy( CopyMovePolicy&&) requires(!allowMove) = delete;
             CopyMovePolicy& operator=( CopyMovePolicy&&) requires(!allowMove) = delete;
        };
    }
}
