import struct
import json
import pywavefront
import random

class Chunk:
	def __init__(self, header, paddingValue, data):
		self.header = header
		self.paddingValue = paddingValue
		self.data = data

	def getBytes(self, offset):
		datalen = offset+len(self.data)
		padding = b''
		if datalen % 4 != 0:
			padding = self.paddingValue * (4 - (datalen % 4))
		return struct.pack("<i", len(self.data) + len(padding)) + self.header + self.data + padding

def createJSONChunk(jsonStr):
	return Chunk(bytes("JSON", "ASCII"), b'\x20', bytes(jsonStr, "UTF-8"))

def createBinaryChunk(binaryData):
	return Chunk(struct.pack("<3sx", bytes("BIN", "ASCII")), b'\x00', binaryData)

def writeGLB(file, chunks):
	data = b''
	for chunk in chunks:
		data += chunk.getBytes(12+len(data))
	f = open(file, 'w+b')
	f.write(struct.pack('<4s', bytes("glTF", "ASCII")))
	f.write(struct.pack('<i', 2))
	f.write(struct.pack('<i', 12+len(data)))
	f.write(data)
	f.close()

def generateAssetInfo():
	return {
		"asset": {
			"generator": "WDK Scene Generator",
			"version": "2.0"
		},
		"scene": 0
	}

def generateMaterials():
	return {
		"materials": [
			{
				"pbrMetallicRoughness": {
					"baseColorFactor": [
						1.0, 1.0, 1.0, 1.0
					]
				}
			}
		]
	}

def spliceArray(arr, sectionStart, sectionSize, stride):
		result = []
		i = sectionStart
		while i < len(arr):
			for j in range(sectionSize):
				result.append(arr[i+j])
			i += stride
		return result

class Mesh:
	def __init__(self):
		self.vertices = []
		self.normals = []
		self.tex_coords = []
		self.indices = []

	def loadOBJ(self, file):
		wavefront = pywavefront.Wavefront(file)
		for matName in wavefront.materials:
			mat = wavefront.materials[matName]
			if mat.vertex_format == 'V3F':
				self.vertices += mat.vertices
			elif mat.vertex_format == 'N3F_V3F':
				self.normals += spliceArray(mat.vertices, 0, 3, 6)
				self.vertices += spliceArray(mat.vertices, 3, 3, 6)
			elif mat.vertex_format == 'T2F_V3F':
				self.tex_coords += spliceArray(mat.vertices, 0, 2, 5)
				self.vertices += spliceArray(mat.vertices, 2, 3, 5)
			elif mat.vertex_format == 'T2F_N3F_V3F':
				self.tex_coords += spliceArray(mat.vertices, 0, 2, 8)
				self.normals += spliceArray(mat.vertices, 2, 3, 8)
				self.vertices += spliceArray(mat.vertices, 5, 3, 8)
			else:
				print("Unsupported vertex format "+mat.vertex_format)
				continue
			self.indices += list(range(int(len(self.vertices)/3)))

	def getArrays(self):
		result = []
		if len(self.vertices) > 0:
			result.append(("vert", self.vertices))
		if len(self.normals) > 0:
			result.append(("norm", self.normals))
		if len(self.tex_coords) > 0:
			result.append(("tex", self.tex_coords))
		result.append(("idx", self.indices))
		return result

def generateMeshDescriptor(mesh):
	buffers = [{
		"byteLength": 4*(sum([len(arr) for (typename, arr) in mesh.getArrays()]))
	}]

	bufferViews = []
	offset = 0
	for (typename, arr) in mesh.getArrays():
		byteLength = len(arr*4)
		bufferViews.append({
			"buffer": 0,
			"byteOffset": offset,
			"byteLength": byteLength
		})
		offset += byteLength

	accessors = []
	attributes = {}
	indicesAccI = -1
	i = 0
	for (typename, arr) in mesh.getArrays():
		elem_per_vert = 3
		componentTypeName = "VEC3"
		if typename == "tex":
			elem_per_vert = 2
			componentTypeName = "VEC2"
		elif typename == "idx":
			elem_per_vert = 1
			componentTypeName = "SCALAR"
		accessors.append({
			"bufferView": i,
			"componentType": 5125 if typename == "idx" else 5126,
			"count": len(arr)/elem_per_vert,
			"type": componentTypeName
		})
		if typename == "vert":
			attributes["POSITION"] = i
		elif typename == "norm":
			attributes["NORMAL"] = i
		elif typename == "tex":
			attributes["TEXCOORD_0"] = i
		elif typename == "idx":
			indicesAccI = i
		i += 1

	return {
		"buffers": buffers,
		"bufferViews": bufferViews,
		"accessors": accessors,
		"meshes": [
			{

				"primitives": [
					{
						"attributes": attributes,
						"material": 0,
						"indices": indicesAccI
					}
				]
			}
		]
	}

def generateMeshBuffer(mesh):
	buf = b''
	for (typename, arr) in mesh.getArrays():
		if typename == "idx":
			buf += struct.pack('%si' % len(arr), *arr)
		else:
			buf += struct.pack('%sf' % len(arr), *arr)
	return buf

def generateSceneUnitCubeMatrix():
	nodes = []
	xCount = 32
	yCount = 32
	zCount = 32
	for x in range(xCount):
		for y in range(yCount):
			for z in range(zCount):
				nodes.append(
					{
						"mesh": 0,
						"rotation": [
							0.0,
							1.0,
							0.0,
							0.0
						],
						"scale": [
							1.0/xCount,
							1.0/yCount,
							1.0/zCount
						],
						"translation": [
							x * (1.0/xCount),
							y * (1.0/yCount),
							z * (1.0/zCount)
						]
					}
				)

	return nodes

def generateSceneUnitCubeRandom():
	nodes = []
	xCount = 16
	yCount = 16
	zCount = 16
	for x in range(xCount):
		for y in range(yCount):
			for z in range(zCount):
				nodes.append(
					{
						"mesh": 0,
						"rotation": [
							0.0,
							1.0,
							0.0,
							0.0
						],
						"scale": [
							1.0/xCount,
							1.0/yCount,
							1.0/zCount
						],
						"translation": [
							random.random(),
							random.random(),
							random.random()
						]
					}
				)

	return nodes

def generateSceneOneHugeManySmallXSplit():
	nodes = []
	nodes.append(
		{
			"mesh": 0,
			"rotation": [
				0.0,
				1.0,
				0.0,
				0.0
			],
			"scale": [
				0.85,
				0.85,
				0.85
			],
			"translation": [
				0.9,
				0.5,
				0.5
			]
		}
	)
	smallCount = 100
	for i in range(smallCount):
		nodes.append(
			{
				"mesh": 0,
				"rotation": [
					0.0,
					1.0,
					0.0,
					0.0
				],
				"scale": [
					0.025,
					0.025,
					0.025
				],
				"translation": [
					random.random() * 0.4,
					random.random(),
					random.random()
				]
			}
		)

	return nodes

def generateSceneOneHugeManySmallCorner():
	nodes = []
	nodes.append(
		{
			"mesh": 0,
			"rotation": [
				0.0,
				1.0,
				0.0,
				0.0
			],
			"scale": [
				0.85,
				0.85,
				0.85
			],
			"translation": [
				0.9,
				0.9,
				0.9
			]
		}
	)
	smallCount = 100
	for i in range(smallCount):
		nodes.append(
			{
				"mesh": 0,
				"rotation": [
					0.0,
					1.0,
					0.0,
					0.0
				],
				"scale": [
					0.025,
					0.025,
					0.025
				],
				"translation": [
					random.random() * 0.4,
					random.random() * 0.4,
					random.random() * 0.4
				]
			}
		)

	return nodes

def generateSceneOneHugeManySmallLongInterval():
	nodes = []
	nodes.append(
		{
			"mesh": 0,
			"rotation": [
				0.0,
				1.0,
				0.0,
				0.0
			],
			"scale": [
				0.85,
				0.85,
				0.85
			],
			"translation": [
				0.0,
				0.0,
				0.0
			]
		}
	)
	smallCount = 100
	for i in range(smallCount):
		nodes.append(
			{
				"mesh": 0,
				"rotation": [
					0.0,
					1.0,
					0.0,
					0.0
				],
				"scale": [
					0.025,
					0.025,
					0.025
				],
				"translation": [
					i,
					random.random() * 0.4,
					random.random() * 0.4
				]
			}
		)

	return nodes

def generateScene():
	mesh = Mesh()
	mesh.loadOBJ("/media/wouter/NVME/CGProject/models/sphere.obj")

	nodes = generateSceneOneHugeManySmallLongInterval()

	nodes.append({
		"camera": 0,
		"rotation": [
			0.0,
			1.0,
			0.0,
			0.0
		],
		"scale": [
			1,
			1,
			1
		],
		"translation": [
			0.5,0.5,-3
		]
	})
	nodes.append({
		"rotation": [
			0.0,
			1.0,
			0.0,
			0.0
		],
		"scale": [
			1,
			1,
			1
		],
		"translation": [
			0.5,0.5,-3
		],
		"extras": {
			"IsPointLight": 1.0,
			"LightIntensity": 100
		}
	})
	sceneJson = {
		"nodes": nodes,
		"scene": 0,
		"scenes": [
			{
				"nodes": list(range(len(nodes)))
			}
		],
		"cameras": [
			{
				"type": "perspective",
				"perspective": {
					"aspectRatio": 1.0,
					"yfov": .40,
					"zfar": 100,
					"znear": 0.1
				}
			},
		]
	}
	return [mesh, sceneJson]

def generateGLB():
	gltf = {}
	gltf.update(generateAssetInfo())
	gltf.update(generateMaterials())
	mesh, sceneJson = generateScene()
	gltf.update(generateMeshDescriptor(mesh))
	gltf.update(sceneJson)

	json_chunk = createJSONChunk(json.dumps(gltf))
	data_chunk = createBinaryChunk(generateMeshBuffer(mesh))
	writeGLB("scene.glb", [json_chunk, data_chunk])

generateGLB()