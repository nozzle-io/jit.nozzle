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
		"description" : "Publish GPU textures via nozzle",
		"digest" : "Creates a named shared texture stream that other processes can receive",
		"tags" : "nozzle, texture, sharing, gpu",
		"style" : "",
		"subpatcher_template" : "",
		"assistshowspatchername" : 0,
		"boxes" : [
			{ "box" : { "id" : "obj-1", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 30, 350, 20], "text" : "bbb.nozzle.send — Publish GPU textures via nozzle" } },
			{ "box" : { "id" : "obj-2", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 55, 400, 20], "text" : "Creates a named shared texture stream for inter-process GPU sharing" } },

			{ "box" : { "id" : "obj-10", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 100, 80, 20], "text" : "publish frame" } },
			{ "box" : { "id" : "obj-11", "maxclass" : "button", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "bang" ], "patching_rect" : [50, 120, 24, 24] } },
			{ "box" : { "id" : "obj-12", "maxclass" : "newobj", "numinlets" : 1, "numoutlets" : 0, "patching_rect" : [50, 160, 350, 22], "text" : "bbb.nozzle.send @name myStream @width 640 @height 480" } },

			{ "box" : { "id" : "obj-20", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 210, 200, 20], "text" : "Set dimensions and reinitialize:" } },
			{ "box" : { "id" : "obj-21", "maxclass" : "message", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "" ], "patching_rect" : [50, 235, 100, 22], "text" : "1920 1080" } },
			{ "box" : { "id" : "obj-22", "maxclass" : "message", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "" ], "patching_rect" : [170, 235, "80, 22"], "text" : "1280 720" } },

			{ "box" : { "id" : "obj-30", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 280, 200, 20], "text" : "Print status to console:" } },
			{ "box" : { "id" : "obj-31", "maxclass" : "message", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "" ], "patching_rect" : [50, 305, 60, 22], "text" : "dump" } },

			{ "box" : { "id" : "obj-40", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 360, 350, 20], "text" : "Attributes: @name (symbol) @width (int) @height (int)" } },
			{ "box" : { "id" : "obj-41", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 385, 350, 40], "text" : "bang → publish one frame\nlist w h → set dimensions\nNote: texture content is GPU-side only (no pixel data in Max)" } },

			{ "box" : { "id" : "obj-50", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 450, 350, 20], "text" : "See also: bbb.nozzle.receive" } }
		],
		"lines" : [
			{ "patchline" : { "source" : [ "obj-11", 0 ], "destination" : [ "obj-12", 0 ] } },
			{ "patchline" : { "source" : [ "obj-21", 0 ], "destination" : [ "obj-12", 0 ] } },
			{ "patchline" : { "source" : [ "obj-22", 0 ], "destination" : [ "obj-12", 0 ] } },
			{ "patchline" : { "source" : [ "obj-31", 0 ], "destination" : [ "obj-12", 0 ] } }
		]
	}
}
