#!BPY

# """
# Name: 'The Final Quest Map (.qmap.tmp)...'
# Blender: 244
# Group: 'Export'
# Tooltip: 'Exports map to intermediate format to be converted to The Final Quest Map (.qmap)'
# """

__author__ = "Adam Sawicki"
__url__ = "http://regedit.gamedev.pl/"
__email__ = "mailto:sawickiap@poczta.onet.pl"
__version__ = "1.0"

__bpydoc__ = """\
Exports an indoor map to intermediate format to be converted to QMAP (The Final Quest Map) map file format.
"""

import Blender


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
	# AutoSmoothAngle
	g_oFile.write("\t\t%f\n" % oMesh.maxSmoothAngle)
	# Czy s¹ UV
	if oMesh.faceUV == True:
		g_oFile.write("\t\t1\n")
	else:
		g_oFile.write("\t\t0\n")
	
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
			g_oFile.write(" %i" % v.index)
			if oMesh.faceUV == True:
				Tex = f.uv[iVertIndex]
				g_oFile.write(" %f,%f" % (Tex[0], Tex[1]))
			iVertIndex = iVertIndex + 1
		g_oFile.write("\n")
	g_oFile.write("\t\t}\n")
	
	# Koniec
	g_oFile.write("\t}\n")

	
def ProcessLampObject(a_oObject):
	global g_oFile, g_iNumLamps
	
	# Obiekt mamy w a_oObject, lampê bêdziemy mieli w oLamp
	oLamp = a_oObject.getData()
	
	# SprawdŸ rodzaj
	iType = oLamp.type;
	if (iType != Blender.Lamp.Types['Lamp']) and (iType != Blender.Lamp.Types['Spot']):
		Warning("Lamp type not supported for lamp \"%s\"" % a_oObject.getName())
		return
	
	g_iNumLamps = g_iNumLamps + 1
	
	# Pocz¹tek
	g_oFile.write("\tlamp %s {\n" % QuoteString(a_oObject.getName()))
	
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
	
	# Dane œwiat³a
	# - Œwiat³o punktowe
	if iType == Blender.Lamp.Types['Lamp']:
		# Typ
		g_oFile.write("\t\tLamp\n")
		# Zasiêg
		g_oFile.write("\t\t%f\n" % oLamp.dist)
		# Kolor
		g_oFile.write("\t\t%f, %f, %f\n" % (oLamp.R, oLamp.G, oLamp.B))
	# - Œwiat³o kierunkowe
	elif iType == Blender.Lamp.Types['Spot']:
		# Typ
		g_oFile.write("\t\tSpot\n")
		# Zasiêg
		g_oFile.write("\t\t%f\n" % oLamp.dist)
		# Kolor
		g_oFile.write("\t\t%f, %f, %f\n" % (oLamp.R, oLamp.G, oLamp.B))
		# Spot Blend (inaczej softness, Bl)
		g_oFile.write("\t\t%f\n" % oLamp.spotBlend)
		# Angle of spot beam (in degrees)
		g_oFile.write("\t\t%f\n" % oLamp.spotSize)
	
	# Koniec
	g_oFile.write("\t}\n")


def ProcessObject(a_oObject):
	if a_oObject.parent != None:
		Warning("Object \"%s\" has a parent - not supported." % a_oObject.getName())
	sType = a_oObject.getType()
	if sType == "Mesh":
		ProcessMeshObject(a_oObject)
	elif sType == "Lamp":
		ProcessLampObject(a_oObject)
	# Typy, które s¹ ignorowane i wyœwietlaj¹ ostrze¿enie
	elif (sType == "Curve") or (sType == "MBall") or (sType == "Surf") or (sType == "Armature") or (sType == "Lattice"):
		Warning("Object \"" + a_oObject.getName() + "\" of type \"" + sType + "\" ignored.")
	# Typy ca³kowicie ignorowane: Camera, Empty, Wave, unknown


def OnFileSelector(a_sFileName):
	global g_oFile
	global g_iNumWarnings, g_iNumObjects, g_iNumVertices, g_iNumFaces, g_iNumLamps
	global g_sPluginTitle
	# Intro
	print "Exporting to QMAP..."
	g_oFile = open(a_sFileName, "w")
	try:
		try:
			# Zapis do pliku nag³ówka
			g_oFile.write("QMAP TMP 10\n")
		
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
			
			Blender.Window.WaitCursor(False)
			
			print "Exported: Objects=%i, Vertices=%i, Faces=%i, Lamps=%i" % (g_iNumObjects, g_iNumVertices, g_iNumFaces, g_iNumLamps)
			
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

Blender.Window.WaitCursor(True)

g_iNumWarnings = 0
g_iNumObjects = 0
g_iNumVertices = 0
g_iNumFaces = 0
g_iNumLamps = 0
g_sPluginTitle = "The Final Quest Map Export"

Blender.Window.FileSelector(OnFileSelector, "Export QMAP TMP", Blender.sys.makename(ext='.qmap.tmp'));

