{
	"patcher" : {
		"fileversion" : 1,
		"appversion" : { "major" : 8, "minor" : 6, "revision" : 4 },
		"classnamespace" : "box",
		"rect" : [100, 100, 700, 550],
		"bglocked" : 1,
		"openrect" : [0, 0, 0, 0],
		"openinpresentation" : 0,
		"default_fontsize" : 12,
		"default_fontface" : 0,
		"default_fontname" : "Arial",
		"gridonopen" : 2,
		"gridsize" : [15, 15],
		"gridsnaponopen" : 0,
		"objectsnaponopen" : 1,
		"statusbarvisible" : 2,
		"toolbarvisible" : 2,
		"lefttoolbarpinned" : 0,
		"toptoolbarpinned" : 0,
		"righttoolbarpinned" : 0,
		"bottomtoolbarpinned" : 0,
		"toolbars_unpinned_last_save" : 0,
		"tallnewobj" : 0,
		"boxanimatetime" : 200,
		"enablehscroll" : 1,
		"enablevscroll" : 1,
		"devicewidth" : 0,
		"description" : "Publish jit.matrix data via nozzle",
		"digest" : "Accepts jit.matrix input and copies pixel data to a named shared stream",
		"tags" : "nozzle, matrix, sharing, jit",
		"style" : "",
		"subpatcher_template" : "",
		"assistshowspatchername" : 0,
		"boxes" : [
			{ "box" : { "id" : "obj-1", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 30, 400, 20], "text" : "jit.nozzle.send — Publish jit.matrix data via nozzle" } },
			{ "box" : { "id" : "obj-2", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 55, 400, 20], "text" : "Accepts jit.matrix input and copies pixel data to a shared stream" } },

			{ "box" : { "id" : "obj-10", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 100, 80, 20], "text" : "input matrix" } },
			{ "box" : { "id" : "obj-11", "maxclass" : "newobj", "numinlets" : 1, "numoutlets" : 0, "patching_rect" : [50, 125, 80, 22], "text" : "jit.matrix 4 char 640 480" } },
			{ "box" : { "id" : "obj-12", "maxclass" : "newobj", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "" ], "patching_rect" : [50, 160, 320, 22], "text" : "jit.nozzle.send @name myStream" } },

			{ "box" : { "id" : "obj-20", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 210, 250, 20], "text" : "Output: width height frame_index on publish" } },
			{ "box" : { "id" : "obj-21", "maxclass" : "newobj", "numinlets" : 1, "numoutlets" : 0, "patching_rect" : [50, 235, 80, 22], "text" : "print frame" } },

			{ "box" : { "id" : "obj-30", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 280, 200, 20], "text" : "Print status to console:" } },
			{ "box" : { "id" : "obj-31", "maxclass" : "message", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "" ], "patching_rect" : [50, 305, 60, 22], "text" : "dump" } },

			{ "box" : { "id" : "obj-40", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 360, 400, 20], "text" : "Attributes: @name (symbol)" } },
			{ "box" : { "id" : "obj-41", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 385, 400, 40], "text" : "jit_matrix → receive jit.matrix and publish to shared stream\nOutput: width height frame_index on left outlet" } },

			{ "box" : { "id" : "obj-50", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 450, 350, 20], "text" : "See also: jit.nozzle.receive" } }
		],
		"lines" : [
			{ "patchline" : { "source" : [ "obj-11", 0 ], "destination" : [ "obj-12", 0 ] } },
			{ "patchline" : { "source" : [ "obj-12", 0 ], "destination" : [ "obj-21", 0 ] } },
			{ "patchline" : { "source" : [ "obj-31", 0 ], "destination" : [ "obj-12", 0 ] } }
		]
	}
}
