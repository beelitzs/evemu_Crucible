/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2008 The EVEmu Team
    For the latest information visit http://evemu.mmoforge.org
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:     Zhur
*/

/*
 * this code has devolved into quite a mess, but it works for now...
 *
*/

#ifndef __CACHEDOBJECTMGR_H_INCL__
#define __CACHEDOBJECTMGR_H_INCL__

#include "common.h"
#include <string>
#include <map>

class PyRep;
class PySubStream;
class PyDict;
class PyCachedObject;
class PyCachedCall;
class PyObject;
class PyBuffer;
class PyCachedObjectDecoder;

class CachedObjectMgr {
public:
    ~CachedObjectMgr();

    //internal utility function to keep maps simpler.
    static std::string OIDToString(const PyRep *objectID);

    bool HaveCached(const std::string &objectID) const;
    bool HaveCached(const PyRep *objectID) const;

    bool IsCacheUpToDate(const PyRep *objectID, uint32 version, uint64 timestamp);

    void InvalidateCache(const PyRep *objectID);

    //bool IsObjectFresh(const std::string &objectID, uint32 version, uint64 timestamp);
    void UpdateCacheFromSS(const std::string &objectID, PySubStream **in_cached_data);
    void UpdateCache(const std::string &objectID, PyRep **in_cached_data);
    void UpdateCache(const PyRep *objectID, PyRep **in_cached_data);

    PyObject *MakeCacheHint(const PyRep *objectID);
    PyObject *MakeCacheHint(const std::string &objectID);

    PyObject *GetCachedObject(const PyRep *objectID);
    PyObject *GetCachedObject(const std::string &objectID);

//OLD CCP FILE BASED ACCESS:
    //PyRep *_MakeCacheHint(const char *oname);
    //void AddCacheHint(const char *oname, const char *key, PyDict *into);

    bool LoadCachedObject(const char *obj_name, PySubStream *into);
    bool LoadCachedFile(const char *filename, const char *oname, PySubStream *into);
    PyCachedObjectDecoder *LoadCachedFile(const char *filename, const char *oname);
    PyCachedCall *LoadCachedCall(const char *filename, const char *oname);   //returns ownership

    //Cache file storage routines:
    bool LoadCachedFromFile(const std::string &cacheDir, const std::string &objectID);
    bool LoadCachedFromFile(const std::string &cacheDir, const PyRep *objectID);
    bool SaveCachedToFile(const std::string &cacheDir, const std::string &objectID) const;
    bool SaveCachedToFile(const std::string &cacheDir, const PyRep *objectID) const;

protected:
    bool LoadCachedObject(PyRep *key, const char *oname, PySubStream *into);
    PyCachedObjectDecoder *LoadCachedObject(const char *obj_name);  //returns ownership
    PyCachedObjectDecoder *LoadCachedObject(PyRep *key, const char *oname); //returns ownership

    //static bool AddCachedFileContents(const char *filename, const char *oname, PySubStream *into);

    void GetCacheFileName(PyRep *key, std::string &into);

    void _UpdateCache(const PyRep *objectID, PyBuffer **buffer);

    class CacheRecord {
    public:
        ~CacheRecord();

        PyObject *EncodeHint() const;

        PyRep *objectID;    //we own this
        uint64 timestamp;
        uint32 version;
        PyBuffer *cache; //we own this.
    };
    std::map<std::string, CacheRecord *> m_cachedObjects;   //we own these pointers
};



class PyCachedObject {
public:
    PyCachedObject();
    ~PyCachedObject();

    void Dump(FILE *into, const char *pfx, bool contents_too = false);
//  bool Decode(PySubStream **ss);   //consumes substream
    PyObject *Encode();
    PyCachedObject *Clone() const;

    //object version tuple:
    /*0*/   uint64 timestamp;
    /*0*/   uint32 version;     //might be swapped with nodeID

    /*1*/ //unknown None
    /*2*/uint32 nodeID;
    /*3*/bool shared;       //not sure

//  PyRep *raw_cache;
    /*4*/PyRep *cache;

    /*5*/bool compressed;   //guess
    /*6*/PyRep *objectID;   //generally a string or tuple.
};

class PyCachedObjectDecoder {
public:
    PyCachedObjectDecoder();
    ~PyCachedObjectDecoder();

    void Dump(FILE *into, const char *pfx, bool contents_too = false);
    bool Decode(PySubStream **ss);   //consumes substream
    PyObject *EncodeHint();

    //object version tuple:
    /*0*/   uint64 timestamp;
    /*0*/   uint32 version;     //might be swapped with nodeID

    /*1*/ //unknown None
    /*2*/uint32 nodeID;
    /*3*/bool shared;       //not sure

    /*4*/PySubStream *cache;

    /*5*/bool compressed;   //guess
    /*6*/PyRep *objectID;   //generally a string or tuple.
};


class PyCachedCall {
public:
    PyCachedCall();
    ~PyCachedCall();

    void Dump(FILE *into, const char *pfx, bool contents_too = false);
    bool Decode(PySubStream **ss);   //consumes substream
    //PyRep *Encode();
    //PyRep *EncodeHint();
    PyCachedCall *Clone() const;

    //rret:
        //objectCaching.CachedMethodCallResult object
        //with ( dict(versionCheck, sessionInfo),
        //       substream (unknown right now)
        //       tuple(version)
        //      )
    //used:
        //win32 time
    //lret
        //the call result directly
        PyRep *result;
    //version:
        //copy of the version tuple in rret
};

//run through the rep, concatenating all the strings together and noting if
//there are a no non-string types in the rep (lists and tuples are OK)
class StringCollapseVisitor : public PyVisitor {
public:
    StringCollapseVisitor()
        : good(true) {}

    EVEMU_INLINE void VisitInteger(const PyInt *rep) { good = false; }
    EVEMU_INLINE void VisitLong(const PyLong *rep) { good = false; }
    EVEMU_INLINE void VisitReal(const PyFloat *rep) { good = false; }
    EVEMU_INLINE void VisitBoolean(const PyBool *rep) { good = false; }
    EVEMU_INLINE void VisitNone(const PyNone *rep) { good = false; }
    EVEMU_INLINE void VisitBuffer(const PyBuffer *rep) { good = false; }
    EVEMU_INLINE void VisitString(const PyString *rep) {
        if(!result.empty())
            result += ".";
        result += rep->content();
    }
    //! PackedRow type visitor
    EVEMU_INLINE void VisitPackedRow(const PyPackedRow *rep) { good = false; }
    //! Object type visitor
    EVEMU_INLINE void VisitObject(const PyObject *rep) { good = false; }
    EVEMU_INLINE void VisitObjectEx(const PyObjectEx *rep) { good = false; }

    EVEMU_INLINE void VisitSubStruct(const PySubStruct *rep) { good = false; }
    EVEMU_INLINE void VisitSubStream(const PySubStream *rep) { good = false; }
    EVEMU_INLINE void VisitChecksumedStream(const PyChecksumedStream *rep) { good = false; }

    EVEMU_INLINE void VisitDict(const PyDict *rep) { good = false; }
    EVEMU_INLINE void VisitList(const PyList *rep)
	{
		PyList::const_iterator cur, end;
		cur = rep->items.begin();
		end = rep->items.end();
		for(; cur != end; cur++)
			(*cur)->visit( this );
	}
    EVEMU_INLINE void VisitTuple(const PyTuple *rep)
	{
		PyTuple::const_iterator cur, end;
		cur = rep->items.begin();
		end = rep->items.end();
		for(; cur != end; cur++)
			(*cur)->visit( this );
	}

    std::string result;
    bool good;
};

#endif
