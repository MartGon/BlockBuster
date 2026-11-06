#include <MapGen.h>

#include <entity/Weapon.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <queue>
#include <random>
#include <set>

using namespace Game::MapGen;

namespace
{
    using RNG = std::mt19937;

    // Palette
    enum Color : unsigned int
    {
        FLOOR_0,
        FLOOR_1,
        FLOOR_2,
        FLOOR_3,
        WALL,
        COVER,
        BLUE_BASE,
        RED_BASE,

        COLOR_COUNT
    };
    const std::vector<glm::u8vec4> palette = {
        {175, 175, 165, 255},
        {150, 158, 150, 255},
        {128, 140, 134, 255},
        {108, 124, 120, 255},
        {78, 78, 90, 255},
        {190, 150, 90, 255},
        {95, 135, 205, 255},
        {205, 95, 95, 255},
    };

    constexpr int OUTSIDE = -1;
    constexpr int MAX_ROOM_CELLS = 3;
    constexpr float LOOP_CHANCE = 0.3f;
    constexpr float OPEN_CHANCE = 0.25f;
    constexpr int MIN_SEAM_CONNECTIONS = 2;
    constexpr int WALL_HEIGHT = 4; // Above the highest floor
    constexpr int DOOR_HALF_WIDTH = 1;
    constexpr int DOOR_MARGIN = 2;
    constexpr int TEAM_SPAWNS = 4;
    constexpr int MIN_OBJECT_DIST = 3;
    constexpr int MAX_STEP = 1; // Max walkable step up, in half blocks. Connectivity mustn't depend on jumping

    struct Column
    {
        int top = -1; // Highest solid block
        bool slope = false; // A slope sits on top of 'top'
        Game::RotType slopeRot = Game::ROT_0;
        unsigned int color = FLOOR_0;
        int room = OUTSIDE;
        bool isWall = false;
        bool isCover = false;
        bool reserved = false; // Doors, ramps and their approaches. Kept clear of cover
    };

    struct Door
    {
        glm::ivec2 cellA;
        glm::ivec2 cellB;
    };

    struct Object
    {
        Entity::GameObject::Type type;
        glm::ivec2 col;
        int teamMask = 0;
    };

    // Directions
    const glm::ivec2 DIRS[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    // Slope's high side for each rotation. ROT_0 rises towards -Z, rotating counter-clockwise
    glm::ivec2 HighSide(Game::RotType rot)
    {
        switch(rot)
        {
        case Game::ROT_90: return {-1, 0};
        case Game::ROT_180: return {0, 1};
        case Game::ROT_270: return {1, 0};
        default: return {0, -1};
        }
    }

    Game::RotType RotForHighSide(glm::ivec2 dir)
    {
        for(auto rot : {Game::ROT_0, Game::ROT_90, Game::ROT_180, Game::ROT_270})
            if(HighSide(rot) == dir)
                return rot;
        return Game::ROT_0;
    }

    int DistSq(glm::ivec2 a, glm::ivec2 b)
    {
        auto d = a - b;
        return d.x * d.x + d.y * d.y;
    }

    class Generator
    {
    public:
        Generator(Params params, RNG& rng) : params{params}, rng{rng}
        {
            W = params.cells.x;
            D = params.cells.y;
            cs = params.cellSize;
            Wb = W * cs + 1;
            Db = D * cs + 1;
            wallTop = (params.maxLevels - 1) * LEVEL_HEIGHT + WALL_HEIGHT;
            columns.resize(Wb * Db);
        }

        std::optional<Result> Run()
        {
            SplitRooms();
            ConnectRooms();

            BuildFloors();
            if(!BuildDoors())
                return std::nullopt;
            BuildCover();
            Symmetrize();

            if(!PlaceObjects())
                return std::nullopt;

            if(!Validate())
                return std::nullopt;

            return ToResult();
        }

    private:

        // #### Random #### \\

        int Uniform(int min, int max)
        {
            return std::uniform_int_distribution<int>{min, max}(rng);
        }

        bool Chance(float p)
        {
            return std::uniform_real_distribution<float>{0.0f, 1.0f}(rng) < p;
        }

        // #### Symmetry #### \\

        glm::ivec2 MirrorCell(glm::ivec2 cell) const
        {
            if(params.symmetry == Symmetry::MIRROR_X)
                return {W - 1 - cell.x, cell.y};
            return {W - 1 - cell.x, D - 1 - cell.y};
        }

        glm::ivec2 MirrorCol(glm::ivec2 col) const
        {
            if(params.symmetry == Symmetry::MIRROR_X)
                return {Wb - 1 - col.x, col.y};
            return {Wb - 1 - col.x, Db - 1 - col.y};
        }

        int MirrorRoom(int room) const
        {
            if(room == OUTSIDE)
                return OUTSIDE;
            return room < halfRooms ? room + halfRooms : room - halfRooms;
        }

        std::pair<int, int> MirrorEdge(std::pair<int, int> edge) const
        {
            std::pair<int, int> mirror = std::minmax(MirrorRoom(edge.first), MirrorRoom(edge.second));
            return mirror;
        }

        // A column is canonical if it's on the first half or it is its own mirror
        bool IsCanonical(glm::ivec2 col) const
        {
            auto m = MirrorCol(col);
            return col.x < m.x || (col.x == m.x && col.y <= m.y);
        }

        // #### Grid #### \\

        Column& Col(glm::ivec2 col)
        {
            return columns[col.y * Wb + col.x];
        }

        bool IsInside(glm::ivec2 col) const
        {
            return col.x >= 0 && col.x < Wb && col.y >= 0 && col.y < Db;
        }

        int CellRoom(glm::ivec2 cell) const
        {
            if(cell.x < 0 || cell.x >= W || cell.y < 0 || cell.y >= D)
                return OUTSIDE;
            return cellRooms[cell.y * W + cell.x];
        }

        // Columns on cell borders touch more than one cell
        std::vector<int> CellsTouching(int v) const
        {
            if(v % cs == 0)
                return {v / cs - 1, v / cs};
            return {v / cs};
        }

        std::set<int> RoomsTouching(glm::ivec2 col) const
        {
            std::set<int> rooms;
            for(auto cx : CellsTouching(col.x))
                for(auto cz : CellsTouching(col.y))
                    rooms.insert(CellRoom({cx, cz}));
            return rooms;
        }

        int FloorTop(int room) const
        {
            return levels[room] * LEVEL_HEIGHT;
        }

        unsigned int FloorColor(int room) const
        {
            if(room == baseRoom)
                return BLUE_BASE;
            if(room == MirrorRoom(baseRoom))
                return RED_BASE;
            return FLOOR_0 + levels[room];
        }

        // #### Layout #### \\

        void SplitRooms()
        {
            auto hw = W / 2;
            cellRooms.assign(W * D, OUTSIDE);
            std::vector<std::pair<glm::ivec2, glm::ivec2>> rects;
            Split({0, 0}, {hw, D}, rects);

            // Half rooms take ids [0, halfRooms). Their mirrors, [halfRooms, 2 * halfRooms)
            halfRooms = rects.size();
            for(auto i = 0; i < halfRooms; i++)
            {
                auto [min, max] = rects[i];
                for(auto x = min.x; x < max.x; x++)
                {
                    for(auto z = min.y; z < max.y; z++)
                    {
                        cellRooms[z * W + x] = i;
                        auto m = MirrorCell({x, z});
                        cellRooms[m.y * W + m.x] = i + halfRooms;
                    }
                }
            }

            levels.resize(halfRooms * 2);
            for(auto i = 0; i < halfRooms; i++)
            {
                levels[i] = Uniform(0, params.maxLevels - 1);
                levels[i + halfRooms] = levels[i];
            }

            // The room furthest from the other team
            baseRoom = CellRoom({0, D / 2});
        }

        void Split(glm::ivec2 min, glm::ivec2 max, std::vector<std::pair<glm::ivec2, glm::ivec2>>& rects)
        {
            auto size = max - min;
            bool mustSplit = size.x > MAX_ROOM_CELLS || size.y > MAX_ROOM_CELLS;
            bool canSplit = size.x >= 2 || size.y >= 2;
            bool split = canSplit && (mustSplit || (size.x * size.y > 2 && Chance(0.5f)));
            if(!split)
            {
                rects.push_back({min, max});
                return;
            }

            bool alongX = size.x >= 2 && (size.y < 2 || size.x > size.y || (size.x == size.y && Chance(0.5f)));
            if(alongX)
            {
                auto at = min.x + Uniform(1, size.x - 1);
                Split(min, {at, max.y}, rects);
                Split({at, min.y}, max, rects);
            }
            else
            {
                auto at = min.y + Uniform(1, size.y - 1);
                Split(min, {max.x, at}, rects);
                Split({min.x, at}, max, rects);
            }
        }

        void ConnectRooms()
        {
            // Find borders between rooms
            std::map<std::pair<int, int>, std::vector<Door>> borders;
            for(auto x = 0; x < W; x++)
            {
                for(auto z = 0; z < D; z++)
                {
                    glm::ivec2 cell{x, z};
                    for(auto next : {cell + glm::ivec2{1, 0}, cell + glm::ivec2{0, 1}})
                    {
                        auto a = CellRoom(cell);
                        auto b = CellRoom(next);
                        if(b == OUTSIDE || a == b)
                            continue;

                        std::pair<int, int> key = std::minmax(a, b);
                        auto door = a == key.first ? Door{cell, next} : Door{next, cell};
                        borders[key].push_back(door);
                    }
                }
            }

            // Bases are never connected directly, so the way to the enemy flag goes through other rooms
            std::pair<int, int> baseEdge = std::minmax(baseRoom, MirrorRoom(baseRoom));
            std::vector<std::pair<int, int>> edges;
            for(auto& [edge, doors] : borders)
                if(edge <= MirrorEdge(edge) && edge != baseEdge)
                    edges.push_back(edge);
            std::shuffle(edges.begin(), edges.end(), rng);

            // Random spanning tree. Mirror edges are accepted in pairs, so the result stays symmetric
            std::vector<int> parent(halfRooms * 2);
            for(auto i = 0; i < parent.size(); i++)
                parent[i] = i;
            std::function<int(int)> find = [&](int i){
                return parent[i] == i ? i : parent[i] = find(parent[i]);
            };

            std::set<std::pair<int, int>> accepted;
            for(auto edge : edges)
            {
                auto mEdge = MirrorEdge(edge);
                if(find(edge.first) != find(edge.second) || find(mEdge.first) != find(mEdge.second))
                {
                    parent[find(edge.first)] = find(edge.second);
                    parent[find(mEdge.first)] = find(mEdge.second);
                    accepted.insert(edge);
                }
            }

            // Extra loops, so most places have more than one route
            for(auto edge : edges)
                if(!accepted.count(edge) && Chance(LOOP_CHANCE))
                    accepted.insert(edge);

            // Several routes between team sides
            auto isSeam = [this](std::pair<int, int> e){
                return (e.first < halfRooms) != (e.second < halfRooms);
            };
            auto seamCount = std::count_if(accepted.begin(), accepted.end(), isSeam);
            for(auto edge : edges)
            {
                if(seamCount >= MIN_SEAM_CONNECTIONS)
                    break;
                if(isSeam(edge) && !accepted.count(edge))
                {
                    accepted.insert(edge);
                    seamCount++;
                }
            }

            // Doors
            for(auto edge : accepted)
            {
                auto mEdge = MirrorEdge(edge);
                auto isBase = [this](int room){ return room == baseRoom || room == MirrorRoom(baseRoom); };
                bool canOpen = levels[edge.first] == levels[edge.second] && !isBase(edge.first) && !isBase(edge.second);
                if(canOpen && Chance(OPEN_CHANCE))
                {
                    openEdges.insert(edge);
                    openEdges.insert(mEdge);
                    continue;
                }

                auto& candidates = borders[edge];
                auto door = candidates[Uniform(0, candidates.size() - 1)];
                doors.push_back(door);

                Door mDoor{MirrorCell(door.cellA), MirrorCell(door.cellB)};
                bool isSameDoor = (mDoor.cellA == door.cellA && mDoor.cellB == door.cellB) ||
                    (mDoor.cellA == door.cellB && mDoor.cellB == door.cellA);
                if(!isSameDoor)
                    doors.push_back(mDoor);
            }
        }

        // #### Geometry #### \\

        void BuildFloors()
        {
            for(auto x = 0; x < Wb; x++)
            {
                for(auto z = 0; z < Db; z++)
                {
                    auto& col = Col({x, z});
                    auto rooms = RoomsTouching({x, z});

                    bool isFloor = false;
                    if(!rooms.count(OUTSIDE))
                    {
                        if(rooms.size() == 1)
                            isFloor = true;
                        else if(rooms.size() == 2)
                            isFloor = openEdges.count({*rooms.begin(), *rooms.rbegin()});
                    }

                    if(isFloor)
                    {
                        auto room = *rooms.begin();
                        col.top = FloorTop(room);
                        col.color = FloorColor(room);
                        col.room = room;
                    }
                    else
                    {
                        // Inner walls are only as tall as needed by the rooms they separate
                        col.top = wallTop;
                        if(!rooms.count(OUTSIDE))
                        {
                            auto highest = *std::max_element(rooms.begin(), rooms.end(), [this](int a, int b){
                                return levels[a] < levels[b];
                            });
                            col.top = FloorTop(highest) + WALL_HEIGHT;
                        }
                        col.color = WALL;
                        col.isWall = true;
                    }
                }
            }
        }

        bool BuildDoors()
        {
            for(auto door : doors)
            {
                auto roomA = CellRoom(door.cellA);
                auto roomB = CellRoom(door.cellB);
                auto high = levels[roomA] >= levels[roomB] ? roomA : roomB;
                auto low = high == roomA ? roomB : roomA;
                auto highCell = high == roomA ? door.cellA : door.cellB;
                auto lowCell = high == roomA ? door.cellB : door.cellA;

                // Door columns, centered on the cells' shared edge
                auto dir = lowCell - highCell;
                glm::ivec2 lateral{dir.y != 0, dir.x != 0};
                glm::ivec2 center;
                if(dir.x != 0)
                    center = {std::max(highCell.x, lowCell.x) * cs, highCell.y * cs + cs / 2};
                else
                    center = {highCell.x * cs + cs / 2, std::max(highCell.y, lowCell.y) * cs};

                auto highTop = FloorTop(high);
                auto rise = highTop - FloorTop(low);
                for(auto l = -DOOR_HALF_WIDTH; l <= DOOR_HALF_WIDTH; l++)
                {
                    auto base = center + lateral * l;
                    auto& threshold = Col(base);
                    threshold.top = highTop;
                    threshold.color = FloorColor(high);
                    threshold.room = high;
                    threshold.isWall = false;
                    threshold.reserved = true;

                    // Ramp down into the lower room. Each slope rises one block towards the door
                    for(auto k = 1; k <= rise; k++)
                    {
                        auto& col = Col(base + dir * k);
                        if(col.isWall)
                            return false;

                        auto top = highTop - k;
                        if(col.top > top)
                            continue; // Another ramp is already higher here

                        col.top = top;
                        col.slope = true;
                        col.slopeRot = RotForHighSide(-dir);
                        col.color = FloorColor(low);
                        col.reserved = true;
                    }

                    // Keep the approaches clear
                    for(auto k = 1; k <= DOOR_MARGIN; k++)
                    {
                        for(auto pos : {base - dir * k, base + dir * (rise + k)})
                            if(IsInside(pos))
                                Col(pos).reserved = true;
                    }
                }
            }

            return true;
        }

        void BuildCover()
        {
            std::vector<int> roomCells(halfRooms * 2, 0);
            for(auto room : cellRooms)
                roomCells[room]++;

            for(auto room = 0; room < halfRooms * 2; room++)
            {
                auto pieces = static_cast<int>(params.coverDensity * roomCells[room] + 0.5f);
                for(auto i = 0; i < pieces; i++)
                {
                    // Pillars block sight. Low walls can be jumped over
                    bool isPillar = Chance(0.4f);
                    glm::ivec2 size = isPillar ? glm::ivec2{2, 2} : (Chance(0.5f) ? glm::ivec2{3, 1} : glm::ivec2{1, 3});
                    auto height = isPillar ? 3 : 1;

                    for(auto attempt = 0; attempt < 10; attempt++)
                    {
                        glm::ivec2 pos{Uniform(1, Wb - 2), Uniform(1, Db - 2)};
                        if(CanPlaceCover(pos, size, room))
                        {
                            for(auto x = 0; x < size.x; x++)
                            {
                                for(auto z = 0; z < size.y; z++)
                                {
                                    auto& col = Col(pos + glm::ivec2{x, z});
                                    col.top += height;
                                    col.color = COVER;
                                    col.isCover = true;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }

        bool CanPlaceCover(glm::ivec2 pos, glm::ivec2 size, int room)
        {
            // Footprint plus a margin of one column, so cover never seals a passage
            for(auto x = -1; x <= size.x; x++)
            {
                for(auto z = -1; z <= size.y; z++)
                {
                    auto p = pos + glm::ivec2{x, z};
                    if(!IsInside(p))
                        return false;

                    auto& col = Col(p);
                    if(col.room != room || col.isWall || col.isCover || col.reserved || col.slope || col.top != FloorTop(room))
                        return false;
                }
            }

            return true;
        }

        // Copy the first half onto the second one
        void Symmetrize()
        {
            for(auto x = 0; x < Wb; x++)
            {
                for(auto z = 0; z < Db; z++)
                {
                    glm::ivec2 pos{x, z};
                    if(IsCanonical(pos))
                        continue;

                    auto col = Col(MirrorCol(pos));
                    col.slopeRot = MirrorRot(col.slopeRot, params.symmetry);
                    col.room = MirrorRoom(col.room);
                    if(col.color == BLUE_BASE)
                        col.color = RED_BASE;
                    else if(col.color == RED_BASE)
                        col.color = BLUE_BASE;
                    Col(pos) = col;
                }
            }
        }

        // #### Objects #### \\

        // Flat columns away from walls, edges and cover
        std::vector<glm::ivec2> GetObjectCandidates(int room)
        {
            std::vector<glm::ivec2> candidates;
            for(auto x = 1; x < Wb - 1; x++)
            {
                for(auto z = 1; z < Db - 1; z++)
                {
                    glm::ivec2 pos{x, z};
                    auto& col = Col(pos);
                    if(col.room != room || col.isWall || col.isCover || col.slope || col.reserved)
                        continue;

                    // Mirrored objects need a distinct mirror
                    if(MirrorCol(pos) == pos)
                        continue;

                    bool isFlat = true;
                    for(auto dir : DIRS)
                    {
                        auto& n = Col(pos + dir);
                        isFlat = isFlat && !n.isWall && !n.isCover && !n.slope && n.top == col.top;
                    }

                    if(isFlat)
                        candidates.push_back(pos);
                }
            }

            return candidates;
        }

        std::vector<glm::ivec2> FilterByDistance(const std::vector<glm::ivec2>& candidates)
        {
            std::vector<glm::ivec2> valid;
            for(auto c : candidates)
            {
                bool isFar = std::all_of(objects.begin(), objects.end(), [c](auto& o){
                    return DistSq(c, o.col) >= MIN_OBJECT_DIST * MIN_OBJECT_DIST;
                });
                if(isFar)
                    valid.push_back(c);
            }
            return valid;
        }

        // The candidate furthest from every placed object
        std::optional<glm::ivec2> PickFarthest(const std::vector<glm::ivec2>& candidates)
        {
            auto valid = FilterByDistance(candidates);
            if(valid.empty())
                return std::nullopt;
            if(objects.empty())
                return valid[Uniform(0, valid.size() - 1)];

            auto best = valid.front();
            auto bestDist = -1;
            for(auto c : valid)
            {
                auto minDist = std::numeric_limits<int>::max();
                for(auto& o : objects)
                    minDist = std::min(minDist, DistSq(c, o.col));

                if(minDist > bestDist)
                {
                    bestDist = minDist;
                    best = c;
                }
            }
            return best;
        }

        // The candidate closest to the center of the room
        std::optional<glm::ivec2> PickCentral(const std::vector<glm::ivec2>& candidates)
        {
            auto valid = FilterByDistance(candidates);
            if(valid.empty())
                return std::nullopt;

            glm::vec2 centroid{0.0f};
            for(auto c : candidates)
                centroid += glm::vec2{c};
            centroid /= (float)candidates.size();

            return *std::min_element(valid.begin(), valid.end(), [centroid](auto a, auto b){
                return glm::length(glm::vec2{a} - centroid) < glm::length(glm::vec2{b} - centroid);
            });
        }

        // Places an object and its mirror
        void PlaceMirrored(Object obj)
        {
            objects.push_back(obj);

            Object mirror = obj;
            mirror.col = MirrorCol(obj.col);
            if(obj.type == Entity::GameObject::FLAG_SPAWN_A)
                mirror.type = Entity::GameObject::FLAG_SPAWN_B;
            if(obj.teamMask == 1)
                mirror.teamMask = 2;
            if(mirror.col != obj.col)
                objects.push_back(mirror);
        }

        bool PlaceObjects()
        {
            using GO = Entity::GameObject;

            // Base: flag and team spawns
            auto baseCandidates = GetObjectCandidates(baseRoom);
            auto flag = PickCentral(baseCandidates);
            if(!flag)
                return false;
            PlaceMirrored({GO::FLAG_SPAWN_A, *flag});

            for(auto i = 0; i < TEAM_SPAWNS; i++)
            {
                auto spawn = PickFarthest(baseCandidates);
                if(!spawn)
                    return false;
                PlaceMirrored({GO::RESPAWN, *spawn, 1});
            }

            // Neutral spawns, one per room. Used by Free for All
            std::vector<int> otherRooms;
            for(auto room = 0; room < halfRooms; room++)
                if(room != baseRoom)
                    otherRooms.push_back(room);

            for(auto room : otherRooms)
                if(auto spawn = PickFarthest(GetObjectCandidates(room)))
                    PlaceMirrored({GO::RESPAWN, *spawn, 3});

            // Domination points. A pair in the biggest room, plus one in the middle if there's a spot
            if(!otherRooms.empty())
            {
                std::vector<int> roomCells(halfRooms, 0);
                for(auto room : cellRooms)
                    if(room < halfRooms)
                        roomCells[room]++;

                auto biggest = *std::max_element(otherRooms.begin(), otherRooms.end(), [&](int a, int b){
                    return roomCells[a] < roomCells[b];
                });
                if(auto point = PickCentral(GetObjectCandidates(biggest)))
                    PlaceMirrored({GO::DOMINATION_POINT, *point});
            }

            std::vector<glm::ivec2> middle;
            for(auto x = 0; x < Wb; x++)
            {
                for(auto z = 0; z < Db; z++)
                {
                    glm::ivec2 pos{x, z};
                    auto& col = Col(pos);
                    if(MirrorCol(pos) == pos && !col.isWall && !col.slope && !col.isCover)
                        middle.push_back(pos);
                }
            }
            if(auto point = PickCentral(middle))
                PlaceMirrored({GO::DOMINATION_POINT, *point});

            bool hasDomination = std::any_of(objects.begin(), objects.end(), [](auto& o){
                return o.type == Entity::GameObject::DOMINATION_POINT;
            });
            if(!hasDomination)
                return false;

            // Pickups
            std::shuffle(otherRooms.begin(), otherRooms.end(), rng);
            const GO::Type pickups[] = {GO::HEALTHPACK, GO::WEAPON_CRATE, GO::GRENADES};
            for(auto i = 0; i < std::size(pickups) && i < otherRooms.size(); i++)
                if(auto pos = PickFarthest(GetObjectCandidates(otherRooms[i])))
                    PlaceMirrored({pickups[i], *pos});

            return true;
        }

        // #### Validation #### \\

        // Height of a column's surface at its edge facing 'dir', in half blocks
        int EdgeHeight(glm::ivec2 pos, glm::ivec2 dir)
        {
            auto& col = Col(pos);
            if(!col.slope)
                return (col.top + 1) * 2;

            auto high = HighSide(col.slopeRot);
            if(dir == high)
                return (col.top + 2) * 2;
            if(dir == -high)
                return (col.top + 1) * 2;
            return (col.top + 1) * 2 + 1;
        }

        bool CanWalk(glm::ivec2 from, glm::ivec2 dir)
        {
            auto to = from + dir;
            if(!IsInside(to))
                return false;

            auto step = EdgeHeight(to, -dir) - EdgeHeight(from, dir);
            return step <= MAX_STEP;
        }

        std::vector<bool> Reachable(glm::ivec2 start, bool reverse)
        {
            std::vector<bool> visited(Wb * Db, false);
            std::queue<glm::ivec2> queue;
            queue.push(start);
            visited[start.y * Wb + start.x] = true;
            while(!queue.empty())
            {
                auto pos = queue.front();
                queue.pop();
                for(auto dir : DIRS)
                {
                    auto next = pos + dir;
                    if(!IsInside(next) || visited[next.y * Wb + next.x])
                        continue;

                    bool canWalk = reverse ? CanWalk(next, -dir) : CanWalk(pos, dir);
                    if(canWalk)
                    {
                        visited[next.y * Wb + next.x] = true;
                        queue.push(next);
                    }
                }
            }
            return visited;
        }

        // Every room and object must be reachable from a spawn, and it must be possible to come back
        bool Validate()
        {
            auto spawn = std::find_if(objects.begin(), objects.end(), [](auto& o){
                return o.type == Entity::GameObject::RESPAWN;
            });
            if(spawn == objects.end())
                return false;

            auto forward = Reachable(spawn->col, false);
            auto backward = Reachable(spawn->col, true);
            auto isConnected = [&](glm::ivec2 pos){
                auto i = pos.y * Wb + pos.x;
                return forward[i] && backward[i];
            };

            for(auto& o : objects)
                if(!isConnected(o.col))
                    return false;

            std::vector<bool> roomConnected(halfRooms * 2, false);
            for(auto x = 0; x < Wb; x++)
            {
                for(auto z = 0; z < Db; z++)
                {
                    auto& col = Col({x, z});
                    if(col.room != OUTSIDE && !col.isWall && isConnected({x, z}))
                        roomConnected[col.room] = true;
                }
            }

            return std::all_of(roomConnected.begin(), roomConnected.end(), [](bool b){return b;});
        }

        // #### Output #### \\

        Result ToResult()
        {
            Result result;
            result.colors = palette;
            result.size = glm::ivec3{Wb, wallTop + 1, Db};

            // Center the map on the origin
            glm::ivec2 offset{-(Wb - 1) / 2, -(Db - 1) / 2};
            auto toPos = [offset](glm::ivec2 col, int y){
                return glm::ivec3{col.x + offset.x, y, col.y + offset.y};
            };

            for(auto x = 0; x < Wb; x++)
            {
                for(auto z = 0; z < Db; z++)
                {
                    auto& col = Col({x, z});
                    Game::Display display{Game::DisplayType::COLOR, col.color};
                    for(auto y = 0; y <= col.top; y++)
                        result.blocks.push_back({toPos({x, z}, y), Game::Block{Game::BLOCK, Game::BlockRot{Game::ROT_0, Game::ROT_0}, display}});

                    if(col.slope)
                        result.blocks.push_back({toPos({x, z}, col.top + 1), Game::Block{Game::SLOPE, Game::BlockRot{col.slopeRot, Game::ROT_0}, display}});
                }
            }

            const int weapons[] = {Entity::BATTLE_RIFLE, Entity::SHOTGUN, Entity::SMG, Entity::SNIPER, Entity::GRENADE_LAUNCHER, Entity::ROCKET_LAUNCHER};
            auto weapon = weapons[Uniform(0, std::size(weapons) - 1)];

            glm::vec2 center{(Wb - 1) / 2.0f, (Db - 1) / 2.0f};
            for(auto& o : objects)
            {
                auto go = Entity::GameObject::Create(o.type);
                go.pos = toPos(o.col, Col(o.col).top + 1);

                if(o.type == Entity::GameObject::RESPAWN)
                {
                    // Face the center of the map
                    auto dir = center - glm::vec2{o.col};
                    auto yaw = glm::length(dir) > 0.0f ? glm::degrees(std::atan2(-dir.y, dir.x)) : 0.0f;
                    go.properties["Orientation"].value = yaw;
                    go.properties["TeamId"].value = o.teamMask;
                }
                else if(o.type == Entity::GameObject::WEAPON_CRATE)
                    go.properties["Weapon ID"].value = weapon;

                result.objects.push_back(go);
            }

            return result;
        }

        Params params;
        RNG& rng;

        // Sizes, in cells and blocks
        int W, D, cs, Wb, Db;
        int wallTop;

        // Layout
        int halfRooms = 0;
        int baseRoom = 0;
        std::vector<int> cellRooms;
        std::vector<int> levels;
        std::set<std::pair<int, int>> openEdges;
        std::vector<Door> doors;

        std::vector<Column> columns;
        std::vector<Object> objects;
    };
}

std::optional<Result> Game::MapGen::Generate(Params params)
{
    // Sanitize. Even sizes keep the mirror lines on cell borders and door centers on blocks
    params.cells = glm::clamp(params.cells, MIN_CELLS, MAX_CELLS);
    params.cells.x += params.cells.x % 2;
    params.cellSize = glm::clamp(params.cellSize, MIN_CELL_SIZE, MAX_CELL_SIZE);
    params.cellSize += params.cellSize % 2;
    params.coverDensity = glm::clamp(params.coverDensity, 0.0f, 1.0f);

    // Ramps must fit inside a cell
    auto maxLevels = (params.cellSize - 3) / LEVEL_HEIGHT + 1;
    params.maxLevels = glm::clamp(params.maxLevels, 1, std::min(maxLevels, (int)FLOOR_3 + 1));

    RNG rng{params.seed};
    for(auto attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
    {
        Generator generator{params, rng};
        if(auto result = generator.Run())
            return result;
    }

    return std::nullopt;
}

glm::ivec3 Game::MapGen::MirrorPos(glm::ivec3 pos, Symmetry symmetry)
{
    if(symmetry == Symmetry::MIRROR_X)
        return {-pos.x, pos.y, pos.z};
    return {-pos.x, pos.y, -pos.z};
}

Game::RotType Game::MapGen::MirrorRot(Game::RotType rot, Symmetry symmetry)
{
    if(symmetry == Symmetry::MIRROR_X)
    {
        // Mirroring X swaps the slopes facing -X and +X
        if(rot == Game::ROT_90)
            return Game::ROT_270;
        if(rot == Game::ROT_270)
            return Game::ROT_90;
        return rot;
    }

    return static_cast<Game::RotType>((rot + 2) % Game::ROT_MAX);
}
