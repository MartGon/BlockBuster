#pragma once

namespace BlockBuster::Editor
{
    using BlockData = std::pair<glm::ivec3, Game::Block>;

    enum class MirrorPlane
    {
        XY,
        XZ,
        YZ,
        NOT_XY,
        NOT_XZ,
        NOT_YZ
    };
}