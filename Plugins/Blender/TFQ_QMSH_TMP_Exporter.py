#!BPY

# """
# Name: 'The Final Quest Mesh (.qmsh.tmp)...'
# Blender: 244
# Group: 'Export'
# Tooltip: 'Exports mesh to intermediate format to be converted to The Final Quest Mesh (.qmsh)'
# """

__author__ = "Adam Sawicki"
__url__ = "http://regedit.gamedev.pl/"
__email__ = "mailto:sawickiap@poczta.onet.pl"
__version__ = "1.0"

__bpydoc__ = """\
Exports mesh with skeletal animation to intermediate format to be converted to QMSH (The Final Quest Mesh) model file format.
"""

import Blender
import Blender.Mathutils
import Blender.Window
import Blender.Draw
import Blender.Scene
import Blender.Object
import Blender.Material
import Blender.Mesh
import Blender.Armature
import Blender.BezTriple
import Blender.Window


#HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
# Funkcje

class MyException(Exception):
	pass


# Obejmuje ³añcuch w "" i escapuje ewentualne znaki specjalne wewn¹trz
def QuoteString(s):
	R = ""
	for ch in s:
		if ch == "\r":
			R = R + "\\r"
		elif ch == "\n":
			R = R + "\\n"
		elif (ch == "\"") or (ch == "\'") or (ch == "\\"):
			R = R + "\\" + ch
		elif ch == "\0":
			R = R + "\\0"
		elif ch == "\v":
			R = R + "\\v"
		elif ch == "\b":
			R = R + "\\b"
		elif ch == "\f":
			R = R + "\\f"
		elif ch == "\a":
			R = R + "\\a"
		elif ch == "\t":
			R = R + "\\t"
		else:
			R = R + ch
	return "\"" + R + "\""


def Note(sMsg):
	print "Note: " + sMsg


def Warning(sMsg):
	global g_iNumWarnings
	g_iNumWarnings = g_iNumWarnings + 1
	print "Warning: " + sMsg


# Wypisuje na konsoli b³¹d i rzuca wyj¹tek klasy MyException, ³apany w kodzie ni¿ej
def Error(sMsg):
	print "Error: " + sMsg
	raise MyException


# Konstruuje listê materia³ów
# Niektóre mog¹ byæ None, a indeksy materia³ów bêd¹ te¿ mog³y wykraczaæ poza zakres tej listy.
# Mechanizm tego jest pomotany, bo zale¿nie od bitów z Object.colbits dany materia³
# jest u¿ywany z listy obiektu lub z listy jego danych (siatki)
def MakeMaterialList(a_oObject, a_oMesh):
	iNumMaterials = max(len(a_oObject.getMaterials()), len(a_oMesh.materials))
	if (iNumMaterials > 32):
		Error("Object \"" + a_oObject.getName() + "\" - too many materials.")

	lMaterials = []
	for mi in range(iNumMaterials):
		if ((a_oObject.colbits >> mi) & 0x01) == 1:
			if (mi >= len(a_oObject.getMaterials())):
				lMaterials.append(None)
			else:
				lMaterials.append(a_oObject.getMaterials()[mi])
		else:
			if (mi >= len(a_oMesh.materials)):
				lMaterials.append(None)
			else:
				lMaterials.append(a_oMesh.materials[mi])
	return lMaterials


def ProcessMeshObject(a_oObject):
	global g_oFile, g_iNumObjects, g_iNumVertices, g_iNumFaces
	
	# Obiekt mamy w a_oObject, siatkê bêdziemy mieli w oMesh
	oMesh = a_oObject.getData(mesh=True)
	
	g_iNumObjects = g_iNumObjects + 1
	g_iNumVertices = g_iNumVertices + len(oMesh.verts)
	g_iNumFaces = g_iNumFaces + len(oMesh.faces)
	
	# Pocz¹tek
	g_oFile.write("\tmesh %s {\n" % QuoteString(a_oObject.getName()))
	
	# Dane obiektu
	
	# Pozycja
	Location = a_oObject.getLocation("worldspace")
	g_oFile.write("\t\t%f,%f,%f\n" % (Location[0], Location[1], Location[2]))
	# Orientacja
	Orientation = a_oObject.getEuler("worldspace")
	g_oFile.write("\t\t%f,%f,%f\n" % (Orientation[0], Orientation[1], Orientation[2]))
	# Skalowanie
	Scaling = a_oObject.getSize("worldspace")
	g_oFile.write("\t\t%f,%f,%f\n" % (Scaling[0], Scaling[1], Scaling[2]))
	# Parent Armature i Parent Bone
	if a_oObject.getParent() == None:
		sParent = ""
	else:
		if a_oObject.getParent().getType() != "Armature":
			Warning("Object \"%s\" has a parent." % a_oObject.getName())
		sParent = a_oObject.getParent().getName()
	if a_oObject.getParentBoneName() == None:
		sParentBone = ""
	else:
		sParentBone = a_oObject.getParentBoneName()
	g_oFile.write("\t\t%s %s\n" % (QuoteString(sParent), QuoteString(sParentBone)))
	# AutoSmoothAngle
	g_oFile.write("\t\t%f\n" % oMesh.maxSmoothAngle)
	
	# Materia³y
	
	lMaterials = MakeMaterialList(a_oObject, oMesh)
	g_oFile.write("\t\tmaterials %i {\n" % len(lMaterials))
	for oMaterial in lMaterials:
		if oMaterial == None:
			g_oFile.write("\t\t\t%s\n" % QuoteString(""))
		else:
			g_oFile.write("\t\t\t%s\n" % QuoteString(oMaterial.getName()))
	g_oFile.write("\t\t}\n")
	
	# Wierzcho³ki
	
	g_oFile.write("\t\tvertices %i {\n" % len(oMesh.verts))
	for v in oMesh.verts:
		g_oFile.write("\t\t\t%f,%f,%f\n" % (v.co[0], v.co[1], v.co[2]))
	g_oFile.write("\t\t}\n")
	
	# Œcianki
	
	g_oFile.write("\t\tfaces %i {\n" % len(oMesh.faces))
	for f in oMesh.faces:
		if f.smooth != 0:
			sSmooth = "1"
		else:
			sSmooth = "0"
		g_oFile.write("\t\t\t%i %i %s" % (len(f.verts), f.mat, sSmooth));
		iVertIndex = 0
		for v in f.verts:
			if oMesh.faceUV == True:
				Tex = f.uv[iVertIndex]
			else:
				Tex = Blender.Mathutils.Vector(0, 0)
			g_oFile.write(" %i %f,%f" % (v.index, Tex[0], Tex[1]))
			iVertIndex = iVertIndex + 1
		g_oFile.write("\n")
	g_oFile.write("\t\t}\n")
	
	# Grupy wierzcho³ków
	g_oFile.write("\t\tvertex_groups {\n")
	for sVertGroupName in oMesh.getVertGroupNames():
		g_oFile.write("\t\t\t%s {" % QuoteString(sVertGroupName))
		for iIndex, fWeight in oMesh.getVertsFromGroup(sVertGroupName, 1):
			g_oFile.write(" %i %f" % (iIndex, fWeight))
		g_oFile.write(" }\n")
	g_oFile.write("\t\t}\n")
		
	# Koniec
	g_oFile.write("\t}\n")
	

def ProcessArmatureObject(a_oObject):
	global g_oFile, g_iNumBones, g_iNumArmatures
	oArmature = a_oObject.getData()
	
	# Zakazany Parent
	if a_oObject.getParent() != None:
		Warning("Object \"%s\" has a parent." % a_oObject.getName())

	g_iNumArmatures = g_iNumArmatures + 1
	if g_iNumArmatures > 1:
		Error("Multiple Armature objects found in scene.")
		
	# Pocz¹tek
	g_oFile.write("\tarmature %s {\n" % QuoteString(a_oObject.getName()))
	
	# Pozycja
	Location = a_oObject.getLocation("worldspace")
	g_oFile.write("\t\t%f,%f,%f\n" % (Location[0], Location[1], Location[2]))
	# Orientacja
	Orientation = a_oObject.getEuler("worldspace")
	g_oFile.write("\t\t%f,%f,%f\n" % (Orientation[0], Orientation[1], Orientation[2]))
	# Skalowanie
	Scaling = a_oObject.getSize("worldspace")
	g_oFile.write("\t\t%f,%f,%f\n" % (Scaling[0], Scaling[1], Scaling[2]))

	# Parametry ca³ego Armature
	if oArmature.vertexGroups:
		sVertexGroups = "1"
	else:
		sVertexGroups = "0"
	if oArmature.envelopes:
		sEnvelopes = "1"
	else:
		sEnvelopes = "0"
	g_oFile.write("\t\t%s %s\n" % (sVertexGroups, sEnvelopes))
	
	# Dla kolejnych koœci
	for sBoneKey, oBone in oArmature.bones.items():
		# Sprawdzenie
		if oBone.subdivisions != 1:
			Warning("Bone \"%s\" has %i subdivisions (not supported)." % (sBoneKey, oBone.subdivisions))
		# Pocz¹tek
		g_oFile.write("\t\tbone %s {\n" % QuoteString(oBone.name))
		# Parent
		if oBone.parent == None:
			sBoneParent = ""
		else:
			sBoneParent = oBone.parent.name
		g_oFile.write("\t\t\t%s\n" % QuoteString(sBoneParent))
		# Head
		g_oFile.write("\t\t\thead %f,%f,%f %f,%f,%f %f\n" % (oBone.head['BONESPACE'][0], oBone.head['BONESPACE'][1], oBone.head['BONESPACE'][2], oBone.head['ARMATURESPACE'][0], oBone.head['ARMATURESPACE'][1], oBone.head['ARMATURESPACE'][2], oBone.headRadius))
		# Tail
		g_oFile.write("\t\t\ttail %f,%f,%f %f,%f,%f %f\n" % (oBone.tail['BONESPACE'][0], oBone.tail['BONESPACE'][1], oBone.tail['BONESPACE'][2], oBone.tail['ARMATURESPACE'][0], oBone.tail['ARMATURESPACE'][1], oBone.tail['ARMATURESPACE'][2], oBone.tailRadius))
		# Roll
		g_oFile.write("\t\t\t%f %f\n" % (oBone.roll['BONESPACE'], oBone.roll['ARMATURESPACE']))
		# Length, Weight
		g_oFile.write("\t\t\t%f %f\n" % (oBone.length, oBone.weight))
		# Pusta linia
		g_oFile.write("\n")
		# Macierz w BONESPACE (jest 3x3)
		for iRow in range(3):
			g_oFile.write("\t\t\t%f, %f, %f;\n" % (oBone.matrix['BONESPACE'][iRow][0], oBone.matrix['BONESPACE'][iRow][1], oBone.matrix['BONESPACE'][iRow][2]))
		# Pusta linia
		g_oFile.write("\n")
		# Macierz w ARMATURESPACE (jest 4x4)
		for iRow in range(4):
			g_oFile.write("\t\t\t%f, %f, %f, %f;\n" % (oBone.matrix['ARMATURESPACE'][iRow][0], oBone.matrix['ARMATURESPACE'][iRow][1], oBone.matrix['ARMATURESPACE'][iRow][2], oBone.matrix['ARMATURESPACE'][iRow][3]))
		# Koniec
		g_oFile.write("\t\t}\n")
		# Policz koœci
		g_iNumBones = g_iNumBones + 1
	
	# Koniec
	g_oFile.write("\t}\n")


def ProcessObject(a_oObject):
	sType = a_oObject.getType()
	if sType == "Mesh":
		ProcessMeshObject(a_oObject)
	elif sType == "Armature":
		ProcessArmatureObject(a_oObject)
	# Typy, które s¹ ignorowane i wyœwietlaj¹ ostrze¿enie
	elif (sType == "Curve") or (sType == "Lattice") or (sType == "MBall") or (sType == "Surf"):
		Warning("Object \"" + a_oObject.getName() + "\" of type \"" + sType + "\" ignored.")
	# Typy ca³kowicie ignorowane: Camera, Lamp, Empty, Wave, unknown


def ProcessActions():
	global g_oFile, g_iNumActions
	g_oFile.write("actions {\n")
	for ActionKey, Action in Blender.Armature.NLA.GetActions().items():
		g_oFile.write("\t%s {\n" % QuoteString(ActionKey))
		for Channel, Ipo in Action.getAllChannelIpos().items():
			if Ipo != None:
				g_oFile.write("\t\t%s {\n" % QuoteString(Channel))
				for Curve in Ipo.curves:
					if Curve.interpolation == Blender.IpoCurve.InterpTypes['CONST']:
						sInterpolation = "CONST"
					elif Curve.interpolation == Blender.IpoCurve.InterpTypes['LINEAR']:
						sInterpolation = "LINEAR"
					#elif Curve.interpolation == Blender.IpoCurve.InterpTypes['BEZIER']:
					else:
						sInterpolation = "BEZIER"

					if Curve.extend == Blender.IpoCurve.ExtendTypes['CONST']:
						sExtend = "CONST"
					elif Curve.extend == Blender.IpoCurve.ExtendTypes['EXTRAP']:
						sExtend = "EXTRAP"
					elif Curve.extend == Blender.IpoCurve.ExtendTypes['CYCLIC']:
						sExtend = "CYCLIC"
					#elif Curve.extend == Blender.IpoCurve.ExtendTypes['CYCLIC_EXTRAP']:
					else:
						sExtend = "CYCLIC_EXTRAP"

					g_oFile.write("\t\t\t%s %s %s {" % (QuoteString(Curve.name), sInterpolation, sExtend))
					
					for Triple in Curve.bezierPoints:
						g_oFile.write(" %f,%f" % (Triple.pt[0], Triple.pt[1]))
					
					g_oFile.write(" }\n")
				g_oFile.write("\t\t}\n")
		g_oFile.write("\t}\n")
		g_iNumActions = g_iNumActions + 1
	g_oFile.write("}\n")


def ProcessParams(a_oScene):
	global g_oFile
	
	g_oFile.write("params {\n")
	g_oFile.write("\tfps = %i\n" % a_oScene.render.fps)
	g_oFile.write("}\n")


def OnFileSelector(a_sFileName):
	global g_oFile
	global g_iNumWarnings, g_iNumObjects, g_iNumVertices, g_iNumFaces
	global g_iNumBones, g_iNumArmatures, g_iNumActions
	global g_sPluginTitle
	Blender.Window.WaitCursor(True)
	# Intro
	print "Exporting to QMSH..."
	g_oFile = open(a_sFileName, "w")
	try:
		try:
			# Zapis do pliku nag³ówka
			g_oFile.write("QMSH TMP 10\n")
		
			# Bie¿¹ca scena
			g_oCurrentScene = Blender.Scene.GetCurrent()
			print "Current Scene: " + g_oCurrentScene.getName()
			g_lObjectList = g_oCurrentScene.objects;
			
			# Sprawdzenie czy w EditMode
			if Blender.Window.EditMode():
				Warning("You are in EditMode - changed may not be respected.")
			
			# Przetwórz wszystkie obiekty bie¿acej sceny
			g_oFile.write("objects {\n")
			for oObject in g_lObjectList:
				ProcessObject(oObject)
			g_oFile.write("}\n")
			
			# Przetwórz wszystkie akcje
			ProcessActions()
			
			# Wyeksportuj parametry
			ProcessParams(g_oCurrentScene)
			
			Blender.Window.WaitCursor(False)
			
			print "Exported: Objects=%i, Vertices=%i, Faces=%i, Bones=%i, Actions=%i" % (g_iNumObjects, g_iNumVertices, g_iNumFaces, g_iNumBones, g_iNumActions)
			
			if g_iNumWarnings == 0:
				sEndMessage = "Export succeeded."
			else:
				sEndMessage = "Succeeded with %i warnings." % g_iNumWarnings
			print sEndMessage
			Blender.Draw.PupBlock(g_sPluginTitle, [sEndMessage])
			
		except MyException:
			try:
				g_oFile.close()
				os.remove(g_sFileName)
			except:
				pass
			Blender.Window.WaitCursor(False)
			Blender.Draw.PupBlock(g_sPluginTitle, ["ERROR. Check console."])
	finally:
		try:
			g_oFile.close()
		except:
			pass
	print


#HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
# Kod

g_iNumWarnings = 0
g_iNumObjects = 0
g_iNumVertices = 0
g_iNumFaces = 0
g_iNumBones = 0
g_iNumArmatures = 0
g_iNumActions = 0
g_sPluginTitle = "The Final Quest Mesh Export"

Blender.Window.FileSelector(OnFileSelector, "Export QMSH TMP", Blender.sys.makename(ext='.qmsh.tmp'));

