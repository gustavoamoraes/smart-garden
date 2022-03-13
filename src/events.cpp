#include "events.h"
#include "main.h"

void EventsManager::add (int unixtime, void (*cb)())
{
    int roundToMinute = (unixtime/60)*60;
    events.emplace(roundToMinute, cb);
    updateAlarm ();
}

//Invokes the current closest event and updates the alarm time to the next
void EventsManager::advance ()
{ 
    auto copy = std::multimap<int, void (*)()> (events);
    auto eventsInTime = copy.equal_range(closestTime);
    events.erase(closestTime);

    for (auto event = eventsInTime.first; event != eventsInTime.second; ++event)
    {
        event->second();
    } 

    updateAlarm ();
}

//Removes only the value of the given key
void EventsManager::remove (int key, void (*value)())
{
    auto keyEvents = events.equal_range(closestTime);
    
    for (auto event = keyEvents.first; event != keyEvents.second; ++event)
    {
        if(event->second == value)
            events.erase(event);
    } 
}

//Set rtc alarm to the time of the next event
void EventsManager::updateAlarm ()
{  
    if(!events.empty())
    {
        int n = events.begin()->first;
        DateTime dt = DateTime(n);
        
        if(n == closestTime)
            return;

        closestTime = n;
        rtc.setAlarm(dt, PCF8563_Alarm_hourly);
    }
}