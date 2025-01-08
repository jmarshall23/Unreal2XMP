/**********************************************************************
  IFC 2.3 Header

  Copyright (c) 1997 - 2002 Immersion Corporation

	Permission to use, copy, modify, distribute, and sell this
	software and its documentation may be granted without fee;
	interested parties are encouraged to request permission from
		Immersion Corporation
		801 Fox Lane
		San Jose, CA 95131
		408-467-1900

	IMMERSION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
	INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
	IN NO EVENT SHALL IMMERSION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
	CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
	LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
	NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
	CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  FILE:		ImmProjects.h

  PURPOSE:	CImmProject
               Manages a set of forces in a project.
               There will be a project for each opened IFR file.
			CImmProjects
			   Manages a set of projects

  STARTED:	2/22/99 by Jeff Mallett


  NOTES/REVISIONS:
     3/2/99 jrm (Jeff Mallett): Force-->Feel renaming
	 3/15/99 jrm: __declspec(dllimport/dllexport) the whole class

**********************************************************************/

#ifndef	__IMM_PROJECTS_H
#define __IMM_PROJECTS_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IFCDLL_
#define DLLIFC __declspec(dllimport)
#else
#define DLLIFC __declspec(dllexport)
#endif


#include "IFCErrors.h"
#include "ImmBaseTypes.h"
#include "ImmDevice.h"
#include "ImmCompoundEffect.h"

class CIFRObjectList;
class CImmProjects;


//================================================================
// CImmProject
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmProject
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

	public:

	CImmProject();

	~CImmProject();

	void
	Close();


    //
    // ATTRIBUTES
    //

	public:

	CImmDevice*
	GetDevice() const
		{ return m_pDevice; }

	BOOL
	GetIsOpen() const
		{ return m_pProjectItems != NULL; }

	CImmCompoundEffect *
	GetEffect(
		LPCSTR lpszEffectName
		);

	CImmCompoundEffect *
	GetEffect(
		int nIndex
		);

	int
	GetNumEffects();
	
	int
	GetNumSounds();
	
	LPCSTR
	GetEffectName(
		int nEffectIndex
		);

	LPCSTR
	GetSoundName(
		int nSoundIndex
		);

	LPCSTR
	GetSoundPath(
		LPCSTR lpszSoundName,
		BOOL bAbsPath = TRUE
		);

	DWORD
	GetSoundData(
		LPCSTR lpszSoundName,
		int nParamID = IMM_PARAM_DURATION
		);

	DWORD 
	GetEffectType(
		LPCSTR lpszEffectName
		);

	DWORD
	GetEffectType(
		int nEffectIndex
		);

	LPCSTR
	GetTypeName(
		LPCSTR lpszEffectName
		);

 	LPCSTR
	GetTypeName(
		int nEffectIndex
		);

	//
    // OPERATIONS
    //

	public:

	BOOL
	Start(
		LPCSTR lpszEffectName = NULL, 
		DWORD dwIterations = IMM_EFFECT_DONT_CHANGE,
		DWORD dwFlags = 0,
		CImmDevice* pDevice = NULL
		);

	BOOL
	Stop(
		LPCSTR lpszEffectName = NULL
		);

	BOOL
	OpenFile(
		LPCSTR lpszFilePath,
		CImmDevice *pDevice
		);

	BOOL
	LoadProjectFromResource(
		HMODULE hRsrcModule,
		LPCSTR pRsrcName,
		CImmDevice *pDevice
		);

	BOOL
	LoadProjectFromMemory(
		LPVOID pProjectDef,
		CImmDevice *pDevice
		);

	BOOL
	LoadProjectObjectPointer(
		BYTE *pMem,
		CImmDevice *pDevice
		);

	BOOL
	WriteToFile(
		LPCSTR lpszFilename
		);

	CImmCompoundEffect *
	CreateEffect(
		LPCSTR lpszEffectName,
		CImmDevice* pDevice = NULL,
		DWORD dwNoDownload = 0
		);

	CImmCompoundEffect *
	CreateEffectByIndex(
		int nEffectIndex,
		CImmDevice* pDevice = NULL,
		DWORD dwNoDownload = 0
		);

	CImmCompoundEffect *
	AddEffect(
		LPCSTR lpszEffectName,
		GENERIC_EFFECT_PTR pObject
		);

	BOOL
	AddSoundInfo(
		LPCSTR lpszSoundName,
		LPCSTR lpszAbsolutePath,
		LPCSTR lpszRelativePath,
		DWORD dwDuration = 0, 
		DWORD dwMagnitude = 0,
		DWORD dwStartDelay = 0
		);

	BOOL
	CreateCompound(
		LPCSTR lpszCompoundName
		);

	BOOL
	AddToCompound(
		LPCSTR lpszCompoundName,
		LPCSTR lpszObjectName
		);

	BOOL
	RemoveFromCompound(
		LPCSTR lpszCompoundName,
		LPCSTR lpszObjectName
		);

	BOOL
	RemoveSound(
		LPCSTR lpszSoundName
		);

	BOOL
	RemoveEffect(
		LPCSTR lpszEffectName
		);

	void
	DestroyEffect(
		CImmCompoundEffect *pCompoundEffect
		);
//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

    protected:

	void
	set_next(
		CImmProject *pNext
		)
		{ m_pNext = pNext; }

	CImmProject *
	get_next() const
		{ return m_pNext; }

	BOOL
	load_ifr(
		HIFRPROJECT hProject
		);

	BOOL 
	load_ffe(
		LPCSTR lpszFilePath
		);

	BOOL 
	write_ffe(
		LPCSTR szOutFile
		);

    //
    // FRIENDS
    //

	public:

	friend BOOL 
		CImmEffect::InitializeFromProject(
			CImmProject &project,
			LPCSTR lpszEffectName,
			CImmDevice* pDevice, /* = NULL */
			DWORD dwNoDownload // = 0
		);

	friend class CImmProjects;

    //
    // INTERNAL DATA
    //

	protected:

	CIFRObjectList*  m_pProjectItems;
	CImmDevice* m_pDevice;
	TCHAR m_szProjectFileName[MAX_PATH];

	private:

	CImmProject* m_pNext;
};



//================================================================
// CImmProjects
//================================================================

//
// ------ PUBLIC INTERFACE ------ 
//

class DLLIFC CImmProjects
{
    //
    // CONSTRUCTOR/DESTRUCTOR
    //

	public:

	CImmProjects() : m_pProjects(NULL) { }

	~CImmProjects();

	void
	Close();


    //
    // ATTRIBUTES
    //

	public:

	CImmProject *
	GetProject(
		int index = 0
		);


    //
    // OPERATIONS
    //

	public:

	BOOL
	Stop();

	long
	OpenFile(
		LPCSTR lpszFilePath,
		CImmDevice *pDevice
		);

	long
	LoadProjectFromResource(
		HMODULE hRsrcModule,
		LPCSTR pRsrcName,
		CImmDevice *pDevice
		);

	long
	LoadProjectFromMemory(
		LPVOID pProjectDef,
		CImmDevice *pDevice
		);


//
// ------ PRIVATE INTERFACE ------ 
//

    //
    // HELPERS
    //

    protected:


    //
    // INTERNAL DATA
    //
	protected:

	CImmProject *m_pProjects;
};



#endif // __IMM_PROJECTS_H

