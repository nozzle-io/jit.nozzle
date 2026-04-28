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
		"description" : "Publish OpenGL textures via nozzle",
		"digest" : "Accepts jit_gl_texture input and publishes the GL texture to a named shared stream",
		"tags" : "nozzle, gl, texture, sharing, jit",
		"style" : "",
		"subpatcher_template" : "",
		"assistshowspatchername" : 0,
		"boxes" : [
			{ "box" : { "id" : "obj-1", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 30, 450, 20], "text" : "jit.gl.bbb.nozzle.send — Publish OpenGL textures via nozzle" } },
			{ "box" : { "id" : "obj-2", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 55, 450, 20], "text" : "Accepts jit_gl_texture input and publishes the GL texture to a shared stream" } },

			{ "box" : { "id" : "obj-10", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 100, 80, 20], "text" : "input texture" } },
			{ "box" : { "id" : "obj-11", "maxclass" : "newobj", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "" ], "patching_rect" : [50, 125, 180, 22], "text" : "jit.gl.bbb.nozzle.send @name myStream" } },

			{ "box" : { "id" : "obj-20", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 180, 250, 20], "text" : "Output: width height frame_index on publish" } },
			{ "box" : { "id" : "obj-21", "maxclass" : "newobj", "numinlets" : 1, "numoutlets" : 0, "patching_rect" : [50, 205, 80, 22], "text" : "print frame" } },

			{ "box" : { "id" : "obj-30", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 250, 200, 20], "text" : "Re-publish last cached texture:" } },
			{ "box" : { "id" : "obj-31", "maxclass" : "button", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "bang" ], "patching_rect" : [50, 275, 24, 24] } },

			{ "box" : { "id" : "obj-32", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 320, 200, 20], "text" : "Print status to console:" } },
			{ "box" : { "id" : "obj-33", "maxclass" : "message", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "" ], "patching_rect" : [50, 345, 60, 22], "text" : "dump" } },

			{ "box" : { "id" : "obj-40", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 390, 400, 20], "text" : "Attributes: @name (symbol)" } },
			{ "box" : { "id" : "obj-41", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 415, 450, 40], "text" : "jit_gl_texture → receive jit.gl.texture name and publish to shared stream\nbang → re-publish last cached texture\nOutput: width height frame_index on left outlet" } },

			{ "box" : { "id" : "obj-50", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 480, 350, 20], "text" : "See also: jit.gl.bbb.nozzle.receive" } }
		],
		"lines" : [
			{ "patchline" : { "source" : [ "obj-11", 0 ], "destination" : [ "obj-21", 0 ] } },
			{ "patchline" : { "source" : [ "obj-31", 0 ], "destination" : [ "obj-11", 0 ] } },
			{ "patchline" : { "source" : [ "obj-33", 0 ], "destination" : [ "obj-11", 0 ] } }
		]
	}
}
