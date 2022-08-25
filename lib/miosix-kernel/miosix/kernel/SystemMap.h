#ifndef SYSTEMMAP_H
#define	SYSTEMMAP_H

#include "kernel/sync.h"
#include "config/miosix_settings.h"

#include <map>
#include <string>

#ifdef WITH_PROCESSES

namespace miosix {

class SystemMap
{
public:
    static SystemMap &instance();

    void addElfProgram(const char *name, const unsigned int *elf, unsigned int size);
    void removeElfProgram(const char *name);
    std::pair<const unsigned int*, unsigned int> getElfProgram(const char *name) const;

    unsigned int getElfCount() const;

private:
    SystemMap() {}
    SystemMap(const SystemMap&);
    SystemMap& operator= (const SystemMap&);

    typedef std::map<std::string, std::pair<const unsigned int*, unsigned int> > ProgramsMap;
    ProgramsMap mPrograms;
};

} //namespace miosix

#endif //WITH_PROCESSES

#endif	/* SYSTEMMAP_H */
