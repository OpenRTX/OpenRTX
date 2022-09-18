/***************************************************************************
 *   Copyright (C) 2022 by Alain Carlucci,                                 *
 *                         Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <audio_path.h>
#include <map>
#include <set>

/**
 * \internal
 * Data structure representing an audio path with source, destination and
 * priority.
 */
struct Path
{
    int8_t source      = -1;   ///< Source endpoint of the path.
    int8_t destination = -1;   ///< Destination endpoint of the path.
    int8_t priority    = -1;   ///< Path priority level.

    bool isValid() const
    {
        return (source      != -1) &&
               (destination != -1) &&
               (priority    != -1);
    }

    bool operator<(const Path& other) const
    {
        return (source      < other.source) &&
               (destination < other.destination);
    }

    bool isCompatible(const Path& other) const
    {
        enum AudioSource p1Source = (enum AudioSource) source;
        enum AudioSource p2Source = (enum AudioSource) other.source;
        enum AudioSink   p1Sink   = (enum AudioSink)   destination;
        enum AudioSink   p2Sink   = (enum AudioSink)   other.destination;

        return audio_checkPathCompatibility(p1Source, p1Sink, p2Source, p2Sink);
    }
};

/**
 * \internal
 * Data structure representing an established audio route.
 */
struct Route
{
    Path path;                    ///< Path associated to this route.
    std::set< int > suspendList;  ///< Suspended paths with lower priority.
    std::set< int > suspendBy;    ///< List of paths which suspended this route.

    bool isActive() const
    {
        return suspendBy.empty();
    }
};


static std::set< int >        activePaths;      // IDs of currently active paths.
static std::map< int, Route > routes;           // Route data of currently active paths.
static int                    pathCounter = 1;  // Counter for path ID generation.


pathId audioPath_request(enum AudioSource source, enum AudioSink sink,
                         enum AudioPriority prio)
{
    const Path path{source, sink, prio};
    if (!path.isValid())
        return -1;

    std::set< int > pathsToSuspend;

    // Check if this new path can be activated, otherwise return -1
    for (const auto& i : activePaths)
    {
        const Path& activePath = routes.at(i).path;
        if(path.isCompatible(activePath))
            continue;

        // Not compatible where active one has higher priority
        if (activePath.priority >= path.priority)
            return -1;

        // Active path has lower priority than this new one
        pathsToSuspend.insert(i);
    }

    // New path can be activated
    const int newPathId = pathCounter;
    pathCounter += 1;
    const Route newRoute{path, pathsToSuspend, {}};

    // Move active paths that should be suspended to the suspend-list
    for (const auto& i : pathsToSuspend)
    {
        // Move path to suspended-list
        activePaths.erase(i);

        routes.at(i).suspendBy.insert(newPathId);
    }

    // Set this new path as active
    routes.insert(std::make_pair(newPathId, newRoute));
    activePaths.insert(newPathId);

    return newPathId;
}

enum PathStatus audioPath_getStatus(const pathId id)
{
    const auto it = routes.find(id);

    if(it == routes.end())
        return PATH_CLOSED;

    if(it->second.isActive())
        return PATH_OPEN;

    return PATH_SUSPENDED;
}

void audioPath_release(const pathId id)
{
    auto it = routes.find(id);
    if (it == routes.end())  // Does not exists
        return;

    Route dataToRemove = it->second;
    routes.erase(it);
    activePaths.erase(id);

    // For each path that suspended me
    for (const auto& i : dataToRemove.suspendBy)
    {
        auto& suspendList = routes.at(i).suspendList;

        // Remove myself from its suspend-list
        suspendList.erase(id);

        // Add who I suspended
        for (const auto& j : dataToRemove.suspendList)
            suspendList.insert(j);
    }

    // For each path suspended by me
    for (const auto& i : dataToRemove.suspendList)
    {
        auto& suspendBy = routes.at(i).suspendBy;

        // Remove myself
        suspendBy.erase(id);

        if (!dataToRemove.suspendBy.empty())
        {
            // If I was suspended, propagate who suspended me
            for (const auto& j : dataToRemove.suspendBy)
                suspendBy.insert(j);
        }
        else
        {
            // This path can be started again
            if (suspendBy.empty())
                activePaths.insert(i);
        }
    }
}
