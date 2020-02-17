import subprocess

raytracer_exe_path = "/home/wouter/Documents/raytracer/raytracer/cmake-build-release/src/exe/raytracer_exe"
frames_folder = "/media/wouter/Data/Piggybank/gltf_export/"
output_folder = "/media/wouter/Data/Piggybank/render_output/"
start_frame = 1
end_frame = 100

for i in range(start_frame, end_frame+1, 1):
	print("Rendering " + frames_folder + str(i) + ".glb")
	subprocess.call([raytracer_exe_path,
		"--workdir", frames_folder, 
		"--scene", "/" + str(i) + ".glb", 
		"--output", output_folder + str(i),
		"--outputtype", "exr",
		"--width", "1920",
		"--height", "1080",
		#"--soupify"
		#"--pmrayspointlamp", "1E7"
		#"--pmraysarealamp", "1E5"
		#"--aageometry", "4"
		#"--aamaterial", "4"
		#"--savepm",
		#"--pmdepth", "0",
		#"--pmmode", "caustics",
		#"--pmfile", "/home/wouter/Documents/rt/photonmap"
	])
	print("")
	print("#############")
	print("")