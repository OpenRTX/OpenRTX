/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "file_access.h"
#include <vector>
#include <climits>
#include <fcntl.h>
#include "console/console_device.h"
#include "mountpointfs/mountpointfs.h"
#include "fat32/fat32.h"
#include "kernel/logging.h"
#ifdef WITH_PROCESSES
#include "kernel/process.h"
#endif //WITH_PROCESSES

using namespace std;

#ifdef WITH_FILESYSTEM

namespace miosix {

/*
 * A note on the use of strings in this file. This file uses three string
 * types: C string, C++ std::string and StringPart which is an efficent
 * in-place substring of either a C or C++ string.
 * 
 * The functions which are meant to be used by clients of the filesystem
 * API take file names as C strings. This is becase that's the signature
 * of the POSIX calls, such as fopen(), stat(), umount(), ... It is
 * worth noticing that these C strings are const. However, resolving paths
 * requires a writable scratchpad string to be able to remove
 * useless path components, such as "/./", go backwards when a "/../" is
 * found, and follow symbolic links. To this end, all these functions
 * make a copy of the passed string into a temporary C++ string that
 * will be deallocated as soon as the function returns.
 * Resolving a path, however, requires to scan all its path components
 * one by one, checking if the path up to that point is a symbolic link
 * or a mountpoint of a filesystem.
 * For example, resolving "/home/test/file" requires to make three
 * substrings, "/home", "/home/test" and "/home/test/file". Of course,
 * one can simply use the substr() member function of the std::string
 * class. However, this means making lots of tiny memory allocations on
 * the heap, increasing the RAM footprint to resolve a path. To this end,
 * the StringPart class was introduced, to create fast in-place substring
 * of a string.
 */

//
// class FileDescriptorTable
//

FileDescriptorTable::FileDescriptorTable()
    : mutex(FastMutex::RECURSIVE), cwd("/")
{
    FilesystemManager::instance().addFileDescriptorTable(this);
    files[0]=files[1]=files[2]=intrusive_ref_ptr<FileBase>(
        new TerminalDevice(DefaultConsole::instance().get()));
}

FileDescriptorTable::FileDescriptorTable(const FileDescriptorTable& rhs)
    : mutex(FastMutex::RECURSIVE), cwd(rhs.cwd)
{
    //No need to lock the mutex since we are in a constructor and there can't
    //be pointers to this in other threads yet
    for(int i=0;i<MAX_OPEN_FILES;i++) this->files[i]=atomic_load(&rhs.files[i]);
    FilesystemManager::instance().addFileDescriptorTable(this);
}

FileDescriptorTable& FileDescriptorTable::operator=(
        const FileDescriptorTable& rhs)
{
    Lock<FastMutex> l(mutex);
    for(int i=0;i<MAX_OPEN_FILES;i++)
        atomic_store(&this->files[i],atomic_load(&rhs.files[i]));
    return *this;
}

int FileDescriptorTable::open(const char* name, int flags, int mode)
{
    if(name==0 || name[0]=='\0') return -EFAULT;
    Lock<FastMutex> l(mutex);
    for(int i=3;i<MAX_OPEN_FILES;i++)
    {
        if(files[i]) continue;
        //Found an empty file descriptor
        string path=absolutePath(name);
        if(path.empty()) return -ENAMETOOLONG;
        ResolvedPath openData=FilesystemManager::instance().resolvePath(path);
        if(openData.result<0) return openData.result;
        StringPart sp(path,string::npos,openData.off);
        int result=openData.fs->open(files[i],sp,flags,mode);
        if(result==0) return i; //The file descriptor
        else return result; //The error code
    }
    return -ENFILE;
}

int FileDescriptorTable::close(int fd)
{
    //No need to lock the mutex when deleting
    if(fd<0 || fd>=MAX_OPEN_FILES) return -EBADF;
    intrusive_ref_ptr<FileBase> toClose;
    toClose=atomic_exchange(files+fd,intrusive_ref_ptr<FileBase>());
    if(!toClose) return -EBADF; //File entry was not open
    return 0;
}

void FileDescriptorTable::closeAll()
{
    for(int i=0;i<MAX_OPEN_FILES;i++)
        atomic_exchange(files+i,intrusive_ref_ptr<FileBase>());
}

int FileDescriptorTable::getcwd(char *buf, size_t len)
{
    if(buf==0 || len<2) return -EINVAL; //We don't support the buf==0 extension
    Lock<FastMutex> l(mutex);
    struct stat st;
    if(stat(".",&st) || !S_ISDIR(st.st_mode)) return -ENOENT;
    if(cwd.length()>len) return -ERANGE;
    strncpy(buf,cwd.c_str(),len);
    if(cwd.length()>1) buf[cwd.length()-1]='\0'; //Erase last '/' in cwd
    return 0;
}

int FileDescriptorTable::chdir(const char* name)
{
    if(name==0 || name[0]=='\0') return -EFAULT;
    size_t len=strlen(name);
    if(name[len-1]!='/') len++; //Reserve room for trailing slash
    Lock<FastMutex> l(mutex);
    if(name[0]!='/') len+=cwd.length();
    if(len>PATH_MAX) return -ENAMETOOLONG;
    
    string newCwd;
    newCwd.reserve(len);
    if(name[0]=='/') newCwd=name;
    else {
        newCwd=cwd;
        newCwd+=name;
    }
    ResolvedPath openData=FilesystemManager::instance().resolvePath(newCwd);
    if(openData.result<0) return openData.result;
    struct stat st;
    StringPart sp(newCwd,string::npos,openData.off);
    if(int result=openData.fs->lstat(sp,&st)) return result;
    if(!S_ISDIR(st.st_mode)) return -ENOTDIR;
    //NOTE: put after resolvePath() as it strips trailing /
    //Also put after lstat() as it fails if path has a trailing slash
    newCwd+='/';
    cwd=newCwd;
    return 0;
}

int FileDescriptorTable::mkdir(const char *name, int mode)
{
    if(name==0 || name[0]=='\0') return -EFAULT;
    string path=absolutePath(name);
    if(path.empty()) return -ENAMETOOLONG;
    ResolvedPath openData=FilesystemManager::instance().resolvePath(path,true);
    if(openData.result<0) return openData.result;
    StringPart sp(path,string::npos,openData.off);
    return openData.fs->mkdir(sp,mode);
}

int FileDescriptorTable::rmdir(const char *name)
{
    if(name==0 || name[0]=='\0') return -EFAULT;
    string path=absolutePath(name);
    if(path.empty()) return -ENAMETOOLONG;
    ResolvedPath openData=FilesystemManager::instance().resolvePath(path,true);
    if(openData.result<0) return openData.result;
    StringPart sp(path,string::npos,openData.off);
    return openData.fs->rmdir(sp);
}

int FileDescriptorTable::unlink(const char *name)
{
    if(name==0 || name[0]=='\0') return -EFAULT;
    string path=absolutePath(name);
    if(path.empty()) return -ENAMETOOLONG;
    return FilesystemManager::instance().unlinkHelper(path);
}

int FileDescriptorTable::rename(const char *oldName, const char *newName)
{
    if(oldName==0 || oldName[0]=='\0') return -EFAULT;
    if(newName==0 || newName[0]=='\0') return -EFAULT;
    string oldPath=absolutePath(oldName);
    string newPath=absolutePath(newName);
    if(oldPath.empty() || newPath.empty()) return -ENAMETOOLONG;
    return FilesystemManager::instance().renameHelper(oldPath,newPath);
}

int FileDescriptorTable::statImpl(const char* name, struct stat* pstat, bool f)
{
    if(name==0 || name[0]=='\0' || pstat==0) return -EFAULT;
    string path=absolutePath(name);
    if(path.empty()) return -ENAMETOOLONG;
    return FilesystemManager::instance().statHelper(path,pstat,f);
}

FileDescriptorTable::~FileDescriptorTable()
{
    FilesystemManager::instance().removeFileDescriptorTable(this);
    //There's no need to lock the mutex and explicitly close files eventually
    //left open, because if there are other threads accessing this while we are
    //being deleted we have bigger problems anyway
}

string FileDescriptorTable::absolutePath(const char* path)
{
    size_t len=strlen(path);
    if(len>PATH_MAX) return "";
    if(path[0]=='/') return path;
    Lock<FastMutex> l(mutex);
    if(len+cwd.length()>PATH_MAX) return "";
    return cwd+path;
}

/**
 * This class implements the path resolution logic
 */
class PathResolution
{
public:
    /**
     * Constructor
     * \param fs map of all mounted filesystems
     */
    PathResolution(const map<StringPart,intrusive_ref_ptr<FilesystemBase> >& fs)
            : filesystems(fs) {}
    
    /**
     * The main purpose of this class, resolve a path
     * \param path inout parameter with the path to resolve. The resolved path
     * will be modified in-place in this string. The path must be absolute and
     * start with a "/". The caller is responsible for that.
     * \param followLastSymlink if true, follow last symlink
     * \return a resolved path
     */
    ResolvedPath resolvePath(string& path, bool followLastSymlink);
    
private:
    /**
     * Handle a /../ in a path
     * \param path path string
     * \param slash path[slash] is the / character after the ..
     * \return 0 on success, a negative number on error
     */
    int upPathComponent(string& path, size_t slash);
    
    /**
     * Handle a normal path component in a path, i.e, a path component
     * that is neither //, /./ or /../
     * \param path path string
     * \param followIfSymlink if true, follow symbolic links
     * \return 0 on success, or a negative number on error
     */
    int normalPathComponent(string& path, bool followIfSymlink);
    
    /**
     * Follow a symbolic link
     * \param path path string. The relative path into the current filesystem 
     * must be a symbolic link (verified by the caller).
     * \return 0 on success, a negative number on failure
     */
    int followSymlink(string& path);
    
    /**
     * Find to which filesystem this path belongs
     * \param path path string.
     * \return 0 on success, a negative number on failure
     */
    int recursiveFindFs(string& path);

    /// Mounted filesystems
    const map<StringPart,intrusive_ref_ptr<FilesystemBase> >& filesystems;
    
    /// Pointer to root filesystem
    intrusive_ref_ptr<FilesystemBase> root;
    
    /// Current filesystem while looking up path
    intrusive_ref_ptr<FilesystemBase> fs;
    
    /// True if current filesystem supports symlinks
    bool syms;
    
    /// path[index] is first unhandled char
    size_t index;
    
    /// path.substr(indexIntoFs) is the relative path to current filesystem
    size_t indexIntoFs;
    
    /// How many components does the relative path have in current fs
    int depthIntoFs;
    
    /// How many symlinks we've found so far
    int linksFollowed;
    
    /// Maximum number of symbolic links to follow (to avoid endless loops)
    static const int maxLinkToFollow=2;
};

ResolvedPath PathResolution::resolvePath(string& path, bool followLastSymlink)
{
    map<StringPart,intrusive_ref_ptr<FilesystemBase> >::const_iterator it;
    it=filesystems.find(StringPart("/"));
    if(it==filesystems.end()) return ResolvedPath(-ENOENT); //should not happen
    root=fs=it->second;
    syms=fs->supportsSymlinks();
    index=1;       //Skip leading /
    indexIntoFs=1; //NOTE: caller must ensure path[0]=='/'
    depthIntoFs=1;
    linksFollowed=0;
    for(;;)
    {
        size_t slash=path.find_first_of('/',index);
        //cout<<path.substr(0,slash)<<endl;
        //Last component (no trailing /)
        if(slash==string::npos) slash=path.length(); //NOTE: one past the last

        if(slash==index)
        {
            //Path component is empty, caused by double slash, remove it
            path.erase(index,1);
        } else if(slash-index==1 && path[index]=='.')
        {
            path.erase(index,2); //Path component is ".", ignore
        } else if(slash-index==2 && path[index]=='.' && path[index+1]=='.')
        {
            int result=upPathComponent(path,slash);
            if(result<0) return ResolvedPath(result);
        } else {
            index=slash+1; //NOTE: if(slash==string::npos) two past the last
            // follow=followLastSymlink for "/link", but is true for "/link/"
            bool follow=index>path.length() ? followLastSymlink : true;
            int result=normalPathComponent(path,follow);
            if(result<0) return ResolvedPath(result);
        }
        //Last component
        if(index>=path.length())
        {
            //Remove trailing /
            size_t last=path.length()-1;
            if(path[last]=='/')
            {
                path.erase(last,1);
                //This may happen if the last path component is a fs
                if(indexIntoFs>path.length()) indexIntoFs=path.length();
            }
            return ResolvedPath(fs,indexIntoFs);
        }
    }
}

int PathResolution::upPathComponent(string& path, size_t slash)
{
    if(index<=1) return -ENOENT; //root dir has no parent
    size_t removeStart=path.find_last_of('/',index-2);
    if(removeStart==string::npos) return -ENOENT; //should not happen
    path.erase(removeStart,slash-removeStart);
    index=removeStart+1;
    //This may happen when merging a path like "/dir/.."
    if(path.empty()) path='/';
    //This may happen if the new last path component is a fs, e.g. "/dev/null/.."
    if(indexIntoFs>path.length()) indexIntoFs=path.length();
    if(--depthIntoFs>0) return 0;
    
    //Depth went to zero, escape current filesystem
    return recursiveFindFs(path);
}

int PathResolution::normalPathComponent(string& path, bool followIfSymlink)
{
    map<StringPart,intrusive_ref_ptr<FilesystemBase> >::const_iterator it;
    it=filesystems.find(StringPart(path,index-1));
    if(it!=filesystems.end())
    {
        //Jumped to a new filesystem. Not stat-ing the path as we're
        //relying on mount not allowing to mount a filesystem on anything
        //but a directory.
        fs=it->second;
        syms=fs->supportsSymlinks();
        indexIntoFs=index>path.length() ? index-1 : index;
        depthIntoFs=1;
        return 0;
    }
    depthIntoFs++;
    if(syms && followIfSymlink)
    {
        struct stat st;
        {
            StringPart sp(path,index-1,indexIntoFs);
            if(int res=fs->lstat(sp,&st)<0) return res;
        }
        if(S_ISLNK(st.st_mode)) return followSymlink(path);
        else if(index<=path.length() && !S_ISDIR(st.st_mode)) return -ENOTDIR;
    }
    return 0;
}

int PathResolution::followSymlink(string& path)
{
    if(++linksFollowed>=maxLinkToFollow) return -ELOOP;
    string target;
    {
        StringPart sp(path,index-1,indexIntoFs);
        if(int res=fs->readlink(sp,target)<0) return res;
    }
    if(target.empty()) return -ENOENT; //Should not happen
    if(target[0]=='/')
    {
        //Symlink is absolute
        size_t newPathLen=target.length()+path.length()-index+1;
        if(newPathLen>PATH_MAX) return -ENAMETOOLONG;
        string newPath;
        newPath.reserve(newPathLen);
        newPath=target;
        if(index<=path.length())
            newPath.insert(newPath.length(),path,index-1,string::npos);
        path.swap(newPath);
        fs=root;
        syms=root->supportsSymlinks();
        index=1;
        indexIntoFs=1;
        depthIntoFs=1;
    } else {
        //Symlink is relative
        size_t removeStart=path.find_last_of('/',index-2);
        size_t newPathLen=path.length()-(index-removeStart-2)+target.length();
        if(newPathLen>PATH_MAX) return -ENAMETOOLONG;
        string newPath;
        newPath.reserve(newPathLen);
        newPath.insert(0,path,0,removeStart+1);
        newPath+=target;
        if(index<=path.length())
            newPath.insert(newPath.length(),path,index-1,string::npos);
        path.swap(newPath);
        index=removeStart+1;
        depthIntoFs--;
    }
    return 0;
}

int PathResolution::recursiveFindFs(string& path)
{
    depthIntoFs=1;
    size_t backIndex=index;
    for(;;)
    {
        backIndex=path.find_last_of('/',backIndex-1);
        if(backIndex==string::npos) return -ENOENT; //should not happpen
        if(backIndex==0)
        {
            fs=root;
            indexIntoFs=1;
            break;
        }
        map<StringPart,intrusive_ref_ptr<FilesystemBase> >::const_iterator it;
        it=filesystems.find(StringPart(path,backIndex));
        if(it!=filesystems.end())
        {
            fs=it->second;
            indexIntoFs=backIndex+1;
            break;
        }
        depthIntoFs++;
    }
    syms=fs->supportsSymlinks();
    return 0;
}

//
// class FilesystemManager
//

FilesystemManager& FilesystemManager::instance()
{
    static FilesystemManager instance;
    return instance;
}

int FilesystemManager::kmount(const char* path, intrusive_ref_ptr<FilesystemBase> fs)
{
    if(path==0 || path[0]=='\0' || !fs) return -EFAULT;
    Lock<FastMutex> l(mutex);
    size_t len=strlen(path);
    if(len>PATH_MAX) return -ENAMETOOLONG;
    string temp(path);
    if(!(temp=="/" && filesystems.empty())) //Skip check when mounting /
    {
        struct stat st;
        if(int result=statHelper(temp,&st,false)) return result;
        if(!S_ISDIR(st.st_mode)) return -ENOTDIR;
        string parent=temp+"/..";
        if(int result=statHelper(parent,&st,false)) return result;
        fs->setParentFsMountpointInode(st.st_ino);
    }
    if(filesystems.insert(make_pair(StringPart(temp),fs)).second==false)
        return -EBUSY; //Means already mounted
    else
        return 0;
}

int FilesystemManager::umount(const char* path, bool force)
{
    typedef
    typename map<StringPart,intrusive_ref_ptr<FilesystemBase> >::iterator fsIt;
    
    if(path==0 || path[0]=='\0') return -ENOENT;
    size_t len=strlen(path);
    if(len>PATH_MAX) return -ENAMETOOLONG;
    Lock<FastMutex> l(mutex); //A reader-writer lock would be better
    fsIt it=filesystems.find(StringPart(path));
    if(it==filesystems.end()) return -EINVAL;
    
    //This finds all the filesystems that have to be recursively umounted
    //to umount the required filesystem. For example, if /path and /path/path2
    //are filesystems, umounting /path must umount also /path/path2
    vector<fsIt> fsToUmount;
    for(fsIt it2=filesystems.begin();it2!=filesystems.end();++it2)
        if(it2->first.startsWith(it->first)) fsToUmount.push_back(it2);
    
    //Now look into all file descriptor tables if there are open files in the
    //filesystems to umount. If there are, return busy. This is an heavy
    //operation given the way the filesystem data structure is organized, but
    //it has been done like this to minimize the size of an entry in the file
    //descriptor table (4 bytes), and because umount happens infrequently.
    //Note that since we are locking the same mutex used by resolvePath(),
    //other threads can't open new files concurrently while we check
    #ifdef WITH_PROCESSES
    list<FileDescriptorTable*>::iterator it3;
    for(it3=fileTables.begin();it3!=fileTables.end();++it3)
    {
        for(int i=0;i<MAX_OPEN_FILES;i++)
        {
            intrusive_ref_ptr<FileBase> file=(*it3)->getFile(i);
            if(!file) continue;
            vector<fsIt>::iterator it4;
            for(it4=fsToUmount.begin();it4!=fsToUmount.end();++it4)
            {
                if(file->getParent()!=(*it4)->second) continue;
                if(force==false) return -EBUSY;
                (*it3)->close(i); //If forced umount, close the file
            }
        }
    }
    #else //WITH_PROCESSES
    for(int i=0;i<MAX_OPEN_FILES;i++)
    {
        intrusive_ref_ptr<FileBase> file=getFileDescriptorTable().getFile(i);
        if(!file) continue;
        vector<fsIt>::iterator it4;
        for(it4=fsToUmount.begin();it4!=fsToUmount.end();++it4)
        {
            if(file->getParent()!=(*it4)->second) continue;
            if(force==false) return -EBUSY;
            getFileDescriptorTable().close(i);//If forced umount, close the file
        }
    }
    #endif //WITH_PROCESSES
    
    //Now there should be no more files belonging to the filesystems to umount,
    //but check if it is really so, as there is a possible race condition
    //which is the read/close,umount where one thread performs a read (or write)
    //operation on a file descriptor, it gets preempted and another thread does
    //a close on that descriptor and an umount of the filesystem. Also, this may
    //happen in case of a forced umount. In such a case there is no entry in
    //the descriptor table (as close was called) but the operation is still
    //ongoing.
    vector<fsIt>::iterator it5;
    const int maxRetry=3; //Retry up to three times
    for(int i=0;i<maxRetry;i++)
    { 
        bool failed=false;
        for(it5=fsToUmount.begin();it5!=fsToUmount.end();++it5)
        {
            if((*it5)->second->areAllFilesClosed()) continue;
            if(force==false) return -EBUSY;
            failed=true;
            break;
        }
        if(!failed) break;
        if(i==maxRetry-1) return -EBUSY; //Failed to umount even if forced
        Thread::sleep(1000); //Wait to see if the filesystem operation completes
    }
    
    //It is now safe to umount all filesystems
    for(it5=fsToUmount.begin();it5!=fsToUmount.end();++it5)
        filesystems.erase(*it5);
    return 0;
}

void FilesystemManager::umountAll()
{
    Lock<FastMutex> l(mutex);
    #ifdef WITH_PROCESSES
    list<FileDescriptorTable*>::iterator it;
    for(it=fileTables.begin();it!=fileTables.end();++it) (*it)->closeAll();
    #else //WITH_PROCESSES
    getFileDescriptorTable().closeAll();
    #endif //WITH_PROCESSES
    filesystems.clear();
}

ResolvedPath FilesystemManager::resolvePath(string& path, bool followLastSymlink)
{
    //see man path_resolution. This code supports arbitrarily mounted
    //filesystems, symbolic links resolution, but no hardlinks to directories
    if(path.length()>PATH_MAX) return ResolvedPath(-ENAMETOOLONG);
    if(path.empty() || path[0]!='/') return ResolvedPath(-ENOENT);

    Lock<FastMutex> l(mutex);
    PathResolution pr(filesystems);
    return pr.resolvePath(path,followLastSymlink);
}

int FilesystemManager::unlinkHelper(string& path)
{
    //Do everything while keeping the mutex locked to prevent someone to
    //concurrently mount a filesystem on the directory we're unlinking
    Lock<FastMutex> l(mutex);
    ResolvedPath openData=resolvePath(path,true);
    if(openData.result<0) return openData.result;
    //After resolvePath() so path is in canonical form and symlinks are followed
    if(filesystems.find(StringPart(path))!=filesystems.end()) return -EBUSY;
    StringPart sp(path,string::npos,openData.off);
    return openData.fs->unlink(sp);
}

int FilesystemManager::statHelper(string& path, struct stat *pstat, bool f)
{
    ResolvedPath openData=resolvePath(path,f);
    if(openData.result<0) return openData.result;
    StringPart sp(path,string::npos,openData.off);
    return openData.fs->lstat(sp,pstat);
}

int FilesystemManager::renameHelper(string& oldPath, string& newPath)
{
    //Do everything while keeping the mutex locked to prevent someone to
    //concurrently mount a filesystem on the directory we're renaming
    Lock<FastMutex> l(mutex);
    ResolvedPath oldOpenData=resolvePath(oldPath,true);
    if(oldOpenData.result<0) return oldOpenData.result;
    ResolvedPath newOpenData=resolvePath(newPath,true);
    if(newOpenData.result<0) return newOpenData.result;
    
    if(oldOpenData.fs!=newOpenData.fs) return -EXDEV; //Can't rename across fs
    
    //After resolvePath() so path is in canonical form and symlinks are followed
    if(filesystems.find(StringPart(oldPath))!=filesystems.end()) return -EBUSY;
    if(filesystems.find(StringPart(newPath))!=filesystems.end()) return -EBUSY;
    
    StringPart oldSp(oldPath,string::npos,oldOpenData.off);
    StringPart newSp(newPath,string::npos,newOpenData.off);
    
    //Can't rename a directory into a subdirectory of itself
    if(newSp.startsWith(oldSp)) return -EINVAL;
    return oldOpenData.fs->rename(oldSp,newSp);
}

short int FilesystemManager::getFilesystemId()
{
    return atomicAddExchange(&devCount,1);
}

int FilesystemManager::devCount=1;

#ifdef WITH_DEVFS
intrusive_ref_ptr<DevFs> //return value is a pointer to DevFs
#else //WITH_DEVFS
void                     //return value is void
#endif //WITH_DEVFS
basicFilesystemSetup(intrusive_ref_ptr<Device> dev)
{
    bootlog("Mounting MountpointFs as / ... ");
    FilesystemManager& fsm=FilesystemManager::instance();
    intrusive_ref_ptr<FilesystemBase> rootFs(new MountpointFs);
    bootlog(fsm.kmount("/",rootFs)==0 ? "Ok\n" : "Failed\n");
    
    #ifdef WITH_DEVFS
    bootlog("Mounting DevFs as /dev ... ");
    StringPart sp("dev");
    int r1=rootFs->mkdir(sp,0755); 
    intrusive_ref_ptr<DevFs> devfs(new DevFs);
    int r2=fsm.kmount("/dev",devfs);
    bool devFsOk=(r1==0 && r2==0);
    bootlog(devFsOk ? "Ok\n" : "Failed\n");
    if(!devFsOk) return devfs;
    fsm.setDevFs(devfs);
    #endif //WITH_DEVFS
    
    bootlog("Mounting Fat32Fs as /sd ... ");
    bool fat32failed=false;
    intrusive_ref_ptr<FileBase> disk;
    #ifdef WITH_DEVFS
    if(dev) devfs->addDevice("sda",dev);
    StringPart sda("sda");
    if(devfs->open(disk,sda,O_RDWR,0)<0) fat32failed=true;
    #else //WITH_DEVFS
    if(dev && dev->open(disk,intrusive_ref_ptr<FilesystemBase>(0),O_RDWR,0)<0)
        fat32failed=true;
    #endif //WITH_DEVFS
    
    intrusive_ref_ptr<Fat32Fs> fat32;
    if(fat32failed==false)
    {
        fat32=new Fat32Fs(disk);
        if(fat32->mountFailed()) fat32failed=true;
    }
    if(fat32failed==false)
    {
        StringPart sd("sd");
        fat32failed=rootFs->mkdir(sd,0755)!=0;
        fat32failed=fsm.kmount("/sd",fat32)!=0;
    }
    bootlog(fat32failed==0 ? "Ok\n" : "Failed\n");
    
    #ifdef WITH_DEVFS
    return devfs;
    #endif //WITH_DEVFS
}

FileDescriptorTable& getFileDescriptorTable()
{
    #ifdef WITH_PROCESSES
    //Something like
    return Thread::getCurrentThread()->getProcess()->getFileTable();
    #else //WITH_PROCESSES
    static FileDescriptorTable fileTable; ///< The only file table
    return fileTable;
    #endif //WITH_PROCESSES
}

} //namespace miosix

#endif //WITH_FILESYSTEM
