#include <map>

class EventsManager
{
  public:
    void add (int unixtime, void (*cb)());
    void advance ();
    void remove (int key, void (*value)());
    void updateAlarm ();
    
    private:
        int closestTime;
        std::multimap<int, void (*)()> events;
};