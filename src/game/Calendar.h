/*
 * Copyright (C) 2005-2012 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MANGOS_CALENDAR_H
#define MANGOS_CALENDAR_H
#include "Policies/Singleton.h"
#include "Common.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"

enum CalendarEventType
{
    CALENDAR_TYPE_RAID              = 0,
    CALENDAR_TYPE_DUNGEON           = 1,
    CALENDAR_TYPE_PVP               = 2,
    CALENDAR_TYPE_MEETING           = 3,
    CALENDAR_TYPE_OTHER             = 4
};

enum CalendarInviteStatus
{
    CALENDAR_STATUS_INVITED         = 0,
    CALENDAR_STATUS_ACCEPTED        = 1,
    CALENDAR_STATUS_DECLINED        = 2,
    CALENDAR_STATUS_CONFIRMED       = 3,
    CALENDAR_STATUS_OUT             = 4,
    CALENDAR_STATUS_STANDBY         = 5,
    CALENDAR_STATUS_SIGNED_UP       = 6,
    CALENDAR_STATUS_NOT_SIGNED_UP   = 7,
    CALENDAR_STATUS_TENTATIVE       = 8,
    CALENDAR_STATUS_REMOVED         = 9     // correct name?
};

// commented flag never showed in sniff but handled by client
enum CalendarFlags
{
    CALENDAR_FLAG_ALL_ALLOWED       = 0x001,
    //CALENDAR_FLAG_SYSTEM            = 0x004,
    //CALENDAR_FLAG_HOLLIDAY          = 0x008,
    CALENDAR_FLAG_INVITES_LOCKED    = 0x010,
    CALENDAR_FLAG_GUILD_ANNOUNCEMENT = 0x040,
    //CALENDAR_FLAG_RAID_LOCKOUT      = 0x080,
    //CALENDAR_FLAG_UNK_PLAYER        = 0x102,
    //CALENDAR_FLAG_RAID_RESET        = 0x200,
    CALENDAR_FLAG_GUILD_EVENT       = 0x400
};

enum CalendarRepeatType
{
    CALENDAR_REPEAT_NEVER           = 0,
    CALENDAR_REPEAT_WEEKLY          = 1,
    CALENDAR_REPEAT_BIWEEKLY        = 2,
    CALENDAR_REPEAT_MONTHLY         = 3
};

enum CalendarSendEventType
{
    CALENDAR_SENDTYPE_GET           = 0,
    CALENDAR_SENDTYPE_ADD           = 1,
    CALENDAR_SENDTYPE_COPY          = 2
};

enum CalendarModerationRank
{
    CALENDAR_RANK_PLAYER            = 0,
    CALENDAR_RANK_MODERATOR         = 1,
    CALENDAR_RANK_OWNER             = 2
};

#define CALENDAR_MAX_INVITES        100

// forward declaration
class WorldPacket;

class CalendarEvent;
class CalendarInvite;
class CalendarMgr;

typedef UNORDERED_MAP<uint64, CalendarInvite*> CalendarInviteMap;
typedef std::list<CalendarInvite*> CalendarInvitesList;
typedef std::list<CalendarEvent*> CalendarEventsList;

class CalendarEvent
{
public:

    CalendarEvent(uint64 eventId, uint64 creatorGUID, uint32 guildId, CalendarEventType type, int32 dungeonId,
        time_t eventTime, uint32 flags, time_t unknownTime, std::string title, std::string description) :
    EventId(eventId), CreatorGuid(creatorGUID), GuildId(guildId), Type(type), DungeonId(dungeonId),
        EventTime(eventTime), Flags(flags), UnknownTime(unknownTime), Title(title),
        Description(description) { }

    CalendarEvent() : EventId(0), CreatorGuid(uint64(0)), GuildId(0), Type(CALENDAR_TYPE_OTHER), DungeonId(-1), EventTime(0),
        Flags(0), UnknownTime(0), Title(""), Description("") { }

    ~CalendarEvent();

    bool IsGuildEvent() const { return Flags & CALENDAR_FLAG_GUILD_EVENT; }
    bool IsGuildAnnouncement() const { return Flags & CALENDAR_FLAG_GUILD_ANNOUNCEMENT; }

    bool AddInvite(CalendarInvite* invite);

    CalendarInviteMap const* GetInviteMap() const { return &m_Invitee; }

    CalendarInvite* GetInviteById(uint64 inviteId);
    CalendarInvite* GetInviteByGuid(ObjectGuid const& guid);

    bool RemoveInviteById(uint64 inviteId, ObjectGuid const& removerGuid);
    void RemoveInviteByGuid(ObjectGuid const& playerGuid);

    uint64 EventId;
    ObjectGuid CreatorGuid;
    uint32 GuildId;
    CalendarEventType Type;
    CalendarRepeatType Repeatable;
    int32 DungeonId;
    time_t EventTime;
    uint32 Flags;
    time_t UnknownTime;
    std::string Title;
    std::string Description;
private:

    CalendarInviteMap m_Invitee;

    CalendarInviteMap::iterator RemoveInviteByItr(CalendarInviteMap::iterator inviteItr);
    void RemoveAllInvite();
};

class CalendarInvite
{
public:

    CalendarInvite() : m_calendarEvent(NULL), InviteId(0), InviteeGuid(uint64(0)), SenderGuid(uint64(0)),
        LastUpdateTime(time(NULL)), Status(CALENDAR_STATUS_INVITED), Rank(CALENDAR_RANK_PLAYER), Text("") {}

    CalendarInvite(CalendarEvent* calendarEvent, uint64 inviteId, ObjectGuid senderGuid, ObjectGuid inviteeGuid, time_t statusTime,
        CalendarInviteStatus status, CalendarModerationRank rank, std::string text);

    ~CalendarInvite() {}

    CalendarEvent const* GetCalendarEvent() const { return m_calendarEvent; }

    uint64 InviteId;
    ObjectGuid InviteeGuid;
    ObjectGuid SenderGuid;
    time_t LastUpdateTime;
    CalendarInviteStatus Status;
    CalendarModerationRank Rank;
    std::string Text;

private:
    CalendarEvent* m_calendarEvent;
};

typedef std::map<uint64, CalendarEvent> CalendarEventStore;

class CalendarMgr : public MaNGOS::Singleton<CalendarMgr, MaNGOS::ClassLevelLockable<CalendarMgr, ACE_Thread_Mutex> >
{
    public:
        CalendarMgr();
        ~CalendarMgr();

    private:
        CalendarEventStore m_EventStore;
        uint64 m_MaxEventId;
        uint64 m_MaxInviteId;
        std::deque<uint32> m_FreeEventIds;
        std::deque<uint32> m_FreeInviteIds;

        uint64 GetNewEventId();
        uint32 GetNewInviteId();

    public:
        CalendarEventsList* GetPlayerEventsList(ObjectGuid const& guid);
        CalendarInvitesList* GetPlayerInvitesList(ObjectGuid const& guid);
        CalendarEvent* AddEvent(ObjectGuid const& guid, std::string title, std::string description, uint32 type, uint32 repeatable, uint32 maxInvites,
            int32 dungeonId, time_t eventTime, time_t unkTime, uint32 flags);

        CalendarInvite* AddInvite(CalendarEvent* event, ObjectGuid const& senderGuid, ObjectGuid const& inviteeGuid, CalendarInviteStatus status, CalendarModerationRank rank, std::string text, time_t statusTime);

        void RemoveEvent(uint64 eventId, ObjectGuid const& remover);
        bool RemoveInvite(uint32 eventId, uint32 invitId, ObjectGuid const& removerGuid);
        void RemovePlayerCalendar(ObjectGuid const& playerGuid);
        void RemoveGuildCalendar(ObjectGuid const& playerGuid, uint32 GuildId);

        void CopyEvent(uint64 eventId, time_t newTime, ObjectGuid const& guid);
        uint32 GetPlayerNumPending(ObjectGuid const& guid);

        CalendarEvent* GetEventById(uint64 eventId)
        {
            CalendarEventStore::iterator itr = m_EventStore.find(eventId);
            return (itr != m_EventStore.end()) ? &itr->second : NULL;
        }

        // sql related
        void LoadFromDB();

        // send data to client function
        void SendCalendarEventInvite(CalendarInvite const* invite);
        void SendCalendarEventInviteAlert(CalendarInvite const* invite);
        void SendCalendarCommandResult(ObjectGuid const& guid, CalendarResponseResult err, char const* param = NULL);
        void SendCalendarEventRemovedAlert(CalendarEvent const* event);
        void SendCalendarEvent(ObjectGuid const& guid, CalendarEvent const* event, uint32 sendType);
        void SendCalendarEventInviteRemoveAlert(ObjectGuid const& guid, CalendarEvent const* event, CalendarInviteStatus status);
        void SendCalendarEventInviteRemove(CalendarInvite const* invite, uint32 flags);
        void SendCalendarEventStatus(CalendarInvite const* invite);
        void SendCalendarClearPendingAction(ObjectGuid const& guid);
        void SendCalendarEventModeratorStatusAlert(CalendarInvite const* invite);
        void SendCalendarEventUpdateAlert(CalendarEvent const* event, time_t oldEventTime);
        void SendCalendarRaidLockoutRemove(ObjectGuid const& guid, DungeonPersistentState const* save);
        void SendCalendarRaidLockoutAdd(ObjectGuid const& guid, DungeonPersistentState const* save);

        void SendPacketToAllEventRelatives(WorldPacket packet, CalendarEvent const* event);
};

#define sCalendarMgr MaNGOS::Singleton<CalendarMgr>::Instance()

#endif
