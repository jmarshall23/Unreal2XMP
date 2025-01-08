=====================================================================
Read Me First -- Getting Started with the Unreal Warfare Engine
=====================================================================

--------------------------------------------------
Obtaining Unreal source and support (confidential)
--------------------------------------------------

Requirements:
* A signed engine licensing deal; talk to
  Mark Rein (mrein@epicgames.com) or Jay Wilbur (jay@wilburfamily.com) 
  for information. Don't ask them technical questions; instead send
  those to the email support lists below.

Steps:

1. Subscribe your team to the Unreal engine support email lists.
   Everyone on the team, send an email as follows:
       to:      majordomo@epicgames.com
       subject: subscribe unedit yourname@youremailaddress.com
   Programmers, also send an email as follows:
       to:      majordomo@epicgames.com
       subject: subscribe unprog yourname@youremailaddress.com

2. Check out the Unreal Developer Network support site:
       http://udn.epicgames.com/
   Use your team's confidential assigned user name and password
   to access the private sections of the site.


3. Also see our public technical Unreal Tournament web site:
       http://unreal.epicgames.com/

4. Also check out the following unofficial Unreal community web
   sites, many of which have excellent tips and tutorials on using
   the Unreal engine.
       http://www.planetunreal.com/unrealed
       http://www.unrealuniverse.com/
       http://www.planetunreal.com/
   Follow the links on these web pages to the tutorial sites.

5. Download the latest source code from our private FTP site.
   Your team has been assigned a confidential user name and password.

--------------------------
Compiling & Running for PC
--------------------------

Requirements:
* A Microsoft Windows operating system.
* Microsoft Visual C++ 6.0 + Service Pack 4 + Processor Pack.
* WinZip 6.0 (from www.winzip.com).
* The Unreal source distribution .zip file (from Epic's private "unedit" FTP site).

Steps:
1. Obtain the Unreal source from Epic's private FTP site.
1. Unzip the Unreal source .zip into \Unreal\System
2. In Microsoft Developer Studio, load the project \Unreal\System\Unreal.dsw.
2. Compile PC by building the "BuildRelease" project
3. cd \Unreal\System
4. ucc make

-------------------------------
Building a trivial sample level
-------------------------------

1. Follow the "Getting started for PC" instructions above, taking either
   the "no UT content" path or the "with UT content" path.
2. cd \Unreal\System
3. UnrealEd.exe
4. In the toolbar on the left side of the screen, under "Builders", hit 
   the "Cylinder" button.  A red cylinder should appear in all four views.
   Note: If you're not sure which button is meant, make a guess and hold the mouse
   over the button, and a tooltip will pop up saying, for example "Cylinder".
5. On the same toolbar, under "CSG", hit the "subtract" button. The 3D view
   should update with a texture.
6. In the 3D view, hold down the right mouse button and move the mouse around,
   to bring the cylinder into view well.
7. In the 3D view, right-click on a wall. In the menu that pops up, choose "Add
   Light Here".
8. In the toolbar across the top of the screen, hit the "Build Geometry" button
   then the "Build Lighting" button. Your level should appear lit.
9. In the menus, choose View / Actor Browser.
10. In the Actor Browser window, expand the "Navigation Point" item.
11. Highlight the "PlayerStart" item.
12. Close the Actor Browser window.
13. In the 3D view, right-click on the floor of the cylinder and choose
	the "Add PlayerStart here" menu option.
14. In the menus, choose File / Save and give your map a name.
15. In the menus, choose Build / Play Level to play-test your map.

---------------------------------------------------
Creating an UnrealScript .uc class and a .u package
---------------------------------------------------

1. See Unreal/SamplePackage for the example code I wrote and examine
   all the files and comments.  This is the minimum you need to do to 
   create an UnrealScript class in a new package.
2. To add this to your list of "packages to be compiled", edit
   UnrealTournament.ini, go to the list of "EditPackage=" lines, and
   add "EditPackage=SamplePackage" at the bottom of the list.
3. del SamplePackage.u (if it exists)
4. ucc make
   This recompiles any nonexistant .u packages listed in EditPackages.
5. Notice the resulting file, Unreal\System\SamplePackage.u.
6. UnrealTournament.exe EngineMap.unr
7. Press TAB, type "summon SamplePackage.SampleClass", and press enter.
8. Notice that an object (an instance of the class) was created and is
   causing a message to be printed on the screen.

--------------------------------------------------------------
Making an UnrealScript/C++ .u/.dll package with native classes
--------------------------------------------------------------

1. See Unreal/SampleNativePackage for the example code I wrote and examine
   the comments; this is the minimum you need to do to create an 
   UnrealScript class in a new package.
2. To add this to your list of "packages to be compiled", edit
   UnrealTournament.ini, go to the list of "EditPackage=" lines, and
   add "EditPackage=SampleNativePackage" at the bottom of the list.
3. del SampleNativePackage.u (if it exists)
   del SampleNativePackage.dll
4. ucc make -nobind
   This causes the package to be recompiled without checking for the
   presence of the appropriate C++ definitions (which don't exist yet).
   If you omit the "nobind", you will get a ucc error reflecting the
   fact that the matching C++ definitions aren't present yet.
5. Notice the resulting file, Unreal\System\SampleNativePackage.u.
6. UnrealTournament.exe EngineMap.unr
7. Press TAB, type "summon samplenativepackage.nativeclass", and press enter.
8. Unreal exits with an error. Make notice of the message you get; this is
   what you will see when you have a native class in a .u file but
   no matching C++ code to go with it!
9. In Visual C++, load Unreal\SampleNativePackage\SampleNativePackage.dsp
   and recompile it.  Read through all the source very thoroughly. 
10. UnrealTournament.exe EngineMap.unr
11. Press TAB, type "summon SampleNativePackage.NativeClass", and press enter.
12. Notice that an object (an instance of the class) was created and is
   causing a message to be printed on the screen.

Also, there is a good tutorial covering native classes on:
	http://www.sleepwalkertc.com/members/blitz/native/native.html
It's a bit out of date (Unreal Tournament era) but covers the basic concepts.

-------
The End
-------
