#############################################################################
# Options:
#
# nmake -f system.mak debug			: builds dependent files (debug build)
# nmake -f system.mak release		: builds dependent files (release build)
# nmake -f system.mak debugstrict	: builds dependent files (debug build, -strict)
# nmake -f system.mak releasestrict	: builds dependent files (release build, -strict)
# nmake -f system.mak clean			: deletes all .u's
#
#############################################################################

debug: MsgDebug BASE
    ..\ucc make -debug -donesound -nobind -noloadwarn -silent

debugstrict: MsgDebug BASE
    ..\ucc make -debug -donesound -nobind -noloadwarn -silent -strict

release: MsgRelease BASE
    ..\ucc make -donesound -nobind -noloadwarn -silent

releasestrict: MsgRelease BASE
    ..\ucc make -donesound -nobind -noloadwarn -silent -strict

clean: MsgClean
	@echo Deleting all scripts...
    @if exist ..\*.u del ..\*.u

BASE: ..\Core.u ..\Deployables.u ..\Editor.u ..\Engine.u ..\Fire.u ..\IpDrv.u ..\Legend.u ..\ParticleSystems.u ..\U2.u ..\Pawns.u ..\UI.u ..\UnrealEd.u ..\UWeb.u ..\Vehicles.u ..\Weapons.u

DEBUG_RebuildAll: Clean DEBUG

RELEASE_RebuildAll: Clean RELEASE

MsgDebug:
    @echo Building scripts (debug)...

MsgRelease:
    @echo Building scripts (release)...

MsgClean:
    @echo Deleting .u files...

..\Core.u: ..\..\Core\Classes\*.uc
    @echo Core
    @if exist ..\Core.u del ..\Core.u

..\Deployables.u: ..\..\Deployables\Classes\*.uc
    @echo Deployables
    @if exist ..\Deployables.u del ..\Deployables.u

..\Editor.u: ..\..\Editor\Classes\*.uc
    @echo Editor
    @if exist ..\Editor.u del ..\Editor.u

..\Engine.u: ..\..\Engine\Classes\*.uc
    @echo Engine
    @if exist ..\Engine.u del ..\Engine.u

..\Fire.u: ..\..\Fire\Classes\*.uc
    @echo Fire
    @if exist ..\Fire.u del ..\Fire.u

..\IpDrv.u: ..\..\IpDrv\Classes\*.uc
    @echo IpDrv
    @if exist ..\IpDrv.u del ..\IpDrv.u

..\Legend.u: ..\..\Legend\Classes\*.uc
    @echo Legend
    @if exist ..\Legend.u del ..\Legend.u

..\ParticleSystems.u: ..\..\ParticleSystems\Classes\*.uc
    @echo ParticleSystems
    @if exist ..\ParticleSystems.u del ..\ParticleSystems.u

..\U2.u: ..\..\U2\Classes\*.uc
    @echo U2
    @if exist ..\U2.u del ..\U2.u

..\Pawns.u: ..\..\Pawns\Classes\*.uc
    @echo Pawns
    @if exist ..\Pawns.u del ..\Pawns.u

..\UI.u: ..\..\UI\Classes\*.uc
    @echo UI
    @if exist ..\UI.u del ..\UI.u

..\UnrealEd.u: ..\..\UnrealEd\Classes\*.uc
    @echo UnrealEd
    @if exist ..\UnrealEd.u del ..\UnrealEd.u

..\UWeb.u: ..\..\UWeb\Classes\*.uc
    @echo UWeb
    @if exist ..\UWeb.u del ..\UWeb.u

..\Vehicles.u: ..\..\Vehicles\Classes\*.uc
    @echo Vehicles
    @if exist ..\Vehicles.u del ..\Vehicles.u

..\Weapons.u: ..\..\Weapons\Classes\*.uc
    @echo Weapons
    @if exist ..\Weapons.u del ..\Weapons.u

