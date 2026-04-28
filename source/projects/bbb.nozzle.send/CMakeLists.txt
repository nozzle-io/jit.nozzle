bbb_add_external(
	DEPS
		${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/nozzle/lib/osx/libnozzle.a
		"-framework Metal"
		"-framework IOSurface"
		"-framework Foundation"
	INCLUDES
		${CMAKE_CURRENT_SOURCE_DIR}/../../../deps/nozzle/include
)
