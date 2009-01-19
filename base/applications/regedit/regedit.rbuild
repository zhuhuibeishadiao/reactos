<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="regedit" type="win32gui" installname="regedit.exe">
	<include base="regedit">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>uuid</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>ole32</library>
	<library>shell32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>shlwapi</library>
	<library>uuid</library>
	<file>about.c</file>
	<file>childwnd.c</file>
	<file>edit.c</file>
	<file>find.c</file>
	<file>framewnd.c</file>
	<file>hexedit.c</file>
	<file>listview.c</file>
	<file>main.c</file>
	<file>regedit.c</file>
	<file>regproc.c</file>
	<file>security.c</file>
	<file>treeview.c</file>
	<file>regedit.rc</file>
	<pch>regedit.h</pch>
</module>
<directory name="clb">
	<xi:include href="clb/clb.rbuild" />
</directory>
</group>
