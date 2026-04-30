{
	"patcher" : {
		"fileversion" : 1,
		"appversion" : { "major" : 8, "minor" : 6, "revision" : 4 },
		"classnamespace" : "box",
		"rect" : [100, 100, 700, 600],
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
		"description" : "Receive OpenGL textures via nozzle",
		"digest" : "Connects to a named sender and outputs received frames as jit_gl_texture",
		"tags" : "nozzle, gl, texture, sharing, jit",
		"style" : "",
		"subpatcher_template" : "",
		"assistshowspatchername" : 0,
		"boxes" : [
			{ "box" : { "id" : "obj-1", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 30, 450, 20], "text" : "jit.gl.nozzle.receive — Receive OpenGL textures via nozzle" } },
			{ "box" : { "id" : "obj-2", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 55, 450, 20], "text" : "Connects to a named sender and outputs received frames as jit_gl_texture" } },

			{ "box" : { "id" : "obj-10", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 100, 80, 20], "text" : "poll for frame" } },
			{ "box" : { "id" : "obj-11", "maxclass" : "button", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "bang" ], "patching_rect" : [50, 120, 24, 24] } },
			{ "box" : { "id" : "obj-12", "maxclass" : "newobj", "numinlets" : 1, "numoutlets" : 2, "outlettype" : [ "", "" ], "patching_rect" : [50, 160, 320, 22], "text" : "jit.gl.nozzle.receive @name myStream @timeout 0" } },

			{ "box" : { "id" : "obj-13", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [380, 160, 250, 20], "text" : "left: jit_gl_texture output" } },
			{ "box" : { "id" : "obj-14", "maxclass" : "newobj", "numinlets" : 1, "numoutlets" : 0, "patching_rect" : [50, 200, 100, 22], "text" : "print texture" } },
			{ "box" : { "id" : "obj-15", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [160, 200, 250, 20], "text" : "outputs jit_gl_texture name on new frame" } },

			{ "box" : { "id" : "obj-16", "maxclass" : "newobj", "numinlets" : 1, "numoutlets" : 0, "patching_rect" : [380, 200, 100, 22], "text" : "print info" } },
			{ "box" : { "id" : "obj-17", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [490, 200, 200, 20], "text" : "right outlet: frame info events" } },

			{ "box" : { "id" : "obj-20", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 250, 200, 20], "text" : "Reconnect to sender:" } },
			{ "box" : { "id" : "obj-21", "maxclass" : "message", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "" ], "patching_rect" : [50, 275, 80, 22], "text" : "connect" } },

			{ "box" : { "id" : "obj-22", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 315, 200, 20], "text" : "Print connected sender info:" } },
			{ "box" : { "id" : "obj-23", "maxclass" : "message", "numinlets" : 1, "numoutlets" : 1, "outlettype" : [ "" ], "patching_rect" : [50, 340, 60, 22], "text" : "info" } },

			{ "box" : { "id" : "obj-30", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 395, 350, 20], "text" : "Attributes: @name (symbol) @timeout (int) @out_name (symbol)" } },
			{ "box" : { "id" : "obj-31", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 420, 450, 40], "text" : "bang → poll for new frame (outputs jit_gl_texture on left outlet)\ndraw → acquire frame and copy to GL texture (use in render context)\nconnect → reconnect to sender\ninfo → print sender details to console" } },

			{ "box" : { "id" : "obj-40", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 490, 350, 20], "text" : "See also: jit.gl.nozzle.send" } },

			{ "box" : { "id" : "obj-50", "maxclass" : "comment", "numinlets" : 0, "numoutlets" : 0, "patching_rect" : [50, 520, 400, 20], "text" : "Tip: use draw message inside a jit.gl.render chain for GL texture output" } }
		],
		"lines" : [
			{ "patchline" : { "source" : [ "obj-11", 0 ], "destination" : [ "obj-12", 0 ] } },
			{ "patchline" : { "source" : [ "obj-12", 0 ], "destination" : [ "obj-14", 0 ] } },
			{ "patchline" : { "source" : [ "obj-12", 1 ], "destination" : [ "obj-16", 0 ] } },
			{ "patchline" : { "source" : [ "obj-21", 0 ], "destination" : [ "obj-12", 0 ] } },
			{ "patchline" : { "source" : [ "obj-23", 0 ], "destination" : [ "obj-12", 0 ] } }
		]
	}
}
