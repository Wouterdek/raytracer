import subprocess

raytracer_exe_path = "/home/wouter/Documents/raytracer/raytracer/cmake-build-release/src/exe/raytracer_exe"
frames_folder = "/media/wouter/Data/Piggybank/gltf_export/bar/"
output_folder = "/media/wouter/Data/Piggybank/render_output/bar/"
start_frame = 165
end_frame = 230

for i in range(start_frame, end_frame+1, 1):
	print("Rendering " + frames_folder + str(i) + ".glb")
	subprocess.call([raytracer_exe_path,
		"--workdir", frames_folder, 
		"--scene", "#WORKDIR#/" + str(i) + ".glb", 
		"--output", output_folder + str(i)+".exr",
		"--outputtype", "exr",
		"--width", "1920",
		"--height", "1080",
		#"--xstart", "0",
		#"--xend", "576",
		#"--ystart", "0",
		#"--yend", "1080",
		#"--soupify",
		#"--pmrayspointlamp", "1E7",
		#"--pmraysarealamp", "1E5",
		"--aageometry", "32",
		"--aamaterial", "64",
		"--loadpm",
		#"--pmdepth", "0",
		"--pmmode", "caustics",
		"--pmfile", "/home/wouter/Documents/rt/photonmap"
	])
	print("")
	print("#############")
	print("")
