/*
 * Kodowanie Windows-1250, koniec wiersza CR+LF, test: Za¿ó³æ gêœl¹ jaŸñ
 * Error - Klasy wyj¹tków do obs³ugi b³êdów
 * Dokumentacja: Patrz plik doc/Error.txt
 * Copyleft (C) 2007 Adam Sawicki
 * Licencja: GNU LGPL
 * Kontakt: mailto:sawickiap@poczta.onet.pl , http://regedit.gamedev.pl/
 */
#include "Base.hpp"
#include "Error.hpp"

#ifndef WIN32
	#include <errno.h>
#endif

#ifdef USE_DIRECTX
	#include <dxerr.h>
	#include <D3DX9Mesh.h> // Dla ID3DXBuffer
#endif

#ifdef USE_AVI_FILE
	#include <vfw.h>
#endif

#ifdef USE_OPENGL
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif


namespace common
{

void Error::Push(const string &msg, const string &file, int line)
{
	if (msg.empty()) return;

	string file_name;
	size_t pos = file.find_last_of("/\\");
	if (pos != string::npos)
		file_name = file.substr(pos+1);
	else
		file_name = file;

	if (file_name.empty() && line == 0)
		m_Msgs.push_back(msg);
	else if (file_name.empty())
		m_Msgs.push_back("[" + IntToStrR(line) + "] " + msg);
	else if (line == 0)
		m_Msgs.push_back("[" + file_name + "] " + msg);
	else
		m_Msgs.push_back("[" + file_name + "," + IntToStrR(line) + "] " + msg);
}

void Error::GetMessage_(string *Msg, const string &Indent, const string &EOL) const
{
	Msg->clear();
	for (
		std::vector<string>::const_reverse_iterator crit = m_Msgs.rbegin();
		crit != m_Msgs.rend();
		++crit)
	{
		if (!Msg->empty())
			*Msg += EOL;
		*Msg += Indent;
		*Msg += *crit;
	}
}
ErrnoError::ErrnoError(int err_code, const string &msg, const string &file, int line)
{
	Push("(errno," + IntToStrR(err_code) + ") " + strerror(err_code));
	Push(msg, file, line);
}

ErrnoError::ErrnoError(const string &msg, const string &file, int line)
{
	Push("(errno," + IntToStrR(errno) + ") " + strerror(errno));
	Push(msg, file, line);
}

#ifdef USE_SDL
	SDLError::SDLError(const string &msg, const string &file, int line)
	{
		Push("(SDLError) " + string(SDL_GetError()));
		Push(msg, file, line);
	}
#endif

#ifdef USE_OPENGL
	OpenGLError::OpenGLError(const string &msg, const string &file, int line)
	{
		Push("(OpenGLError,"+ IntToStr(glGetError()) +") " + string((char*)gluErrorString(glGetError())));
		Push(msg, file, line);
	}
#endif

#ifdef USE_FMOD
	void FmodError::CodeToMessage(FMOD_RESULT code, string *Enum, string *Message)
	{
		switch (code)
		{
		case FMOD_OK:
			if (Enum) *Enum = "FMOD_OK";
			if (Message) *Message = "No errors.";
			break;
		case FMOD_ERR_ALREADYLOCKED:
			if (Enum) *Enum = "FMOD_ERR_ALREADYLOCKED";
			if (Message) *Message = "Tried to call lock a second time before unlock was called.";
			break;
		case FMOD_ERR_BADCOMMAND:
			if (Enum) *Enum = "FMOD_ERR_BADCOMMAND";
			if (Message) *Message = "Tried to call a function on a data type that does not allow this type of functionality (ie calling Sound::lock on a streaming sound).";
			break;
		case FMOD_ERR_CDDA_DRIVERS:
			if (Enum) *Enum = "FMOD_ERR_CDDA_DRIVERS";
			if (Message) *Message = "Neither NTSCSI nor ASPI could be initialised.";
			break;
		case FMOD_ERR_CDDA_INIT:
			if (Enum) *Enum = "FMOD_ERR_CDDA_INIT";
			if (Message) *Message = "An error occurred while initialising the CDDA subsystem.";
			break;
		case FMOD_ERR_CDDA_INVALID_DEVICE:
			if (Enum) *Enum = "FMOD_ERR_CDDA_INVALID_DEVICE";
			if (Message) *Message = "Couldn't find the specified device.";
			break;
		case FMOD_ERR_CDDA_NOAUDIO:
			if (Enum) *Enum = "FMOD_ERR_CDDA_NOAUDIO";
			if (Message) *Message = "No audio tracks on the specified disc.";
			break;
		case FMOD_ERR_CDDA_NODEVICES:
			if (Enum) *Enum = "FMOD_ERR_CDDA_NODEVICES";
			if (Message) *Message = "No CD/DVD devices were found.";
			break;
		case FMOD_ERR_CDDA_NODISC:
			if (Enum) *Enum = "FMOD_ERR_CDDA_NODISC";
			if (Message) *Message = "No disc present in the specified drive.";
			break;
		case FMOD_ERR_CDDA_READ:
			if (Enum) *Enum = "FMOD_ERR_CDDA_READ";
			if (Message) *Message = "A CDDA read error occurred.";
			break;
		case FMOD_ERR_CHANNEL_ALLOC:
			if (Enum) *Enum = "FMOD_ERR_CHANNEL_ALLOC";
			if (Message) *Message = "Error trying to allocate a channel.";
			break;
		case FMOD_ERR_CHANNEL_STOLEN:
			if (Enum) *Enum = "FMOD_ERR_CHANNEL_STOLEN";
			if (Message) *Message = "The specified channel has been reused to play another sound.";
			break;
		case FMOD_ERR_COM:
			if (Enum) *Enum = "FMOD_ERR_COM";
			if (Message) *Message = "A Win32 COM related error occured. COM failed to initialize or a QueryInterface failed meaning a Windows codec or driver was not installed properly.";
			break;
		case FMOD_ERR_DMA:
			if (Enum) *Enum = "FMOD_ERR_DMA";
			if (Message) *Message = "DMA Failure. See debug output for more information.";
			break;
		case FMOD_ERR_DSP_CONNECTION:
			if (Enum) *Enum = "FMOD_ERR_DSP_CONNECTION";
			if (Message) *Message = "DSP connection error. Connection possibly caused a cyclic dependancy.";
			break;
		case FMOD_ERR_DSP_FORMAT:
			if (Enum) *Enum = "FMOD_ERR_DSP_FORMAT";
			if (Message) *Message = "DSP Format error. A DSP unit may have attempted to connect to this network with the wrong format.";
			break;
		case FMOD_ERR_DSP_NOTFOUND:
			if (Enum) *Enum = "FMOD_ERR_DSP_NOTFOUND";
			if (Message) *Message = "DSP connection error. Couldn't find the DSP unit specified.";
			break;
		case FMOD_ERR_DSP_RUNNING:
			if (Enum) *Enum = "FMOD_ERR_DSP_RUNNING";
			if (Message) *Message = "DSP error. Cannot perform this operation while the network is in the middle of running. This will most likely happen if a connection or disconnection is attempted in a DSP callback.";
			break;
		case FMOD_ERR_DSP_TOOMANYCONNECTIONS:
			if (Enum) *Enum = "FMOD_ERR_DSP_TOOMANYCONNECTIONS";
			if (Message) *Message = "DSP connection error. The unit being connected to or disconnected should only have 1 input or output.";
			break;
		case FMOD_ERR_FILE_BAD:
			if (Enum) *Enum = "FMOD_ERR_FILE_BAD";
			if (Message) *Message = "Error loading file.";
			break;
		case FMOD_ERR_FILE_COULDNOTSEEK:
			if (Enum) *Enum = "FMOD_ERR_FILE_COULDNOTSEEK";
			if (Message) *Message = "Couldn't perform seek operation. This is a limitation of the medium (ie netstreams) or the file format.";
			break;
		case FMOD_ERR_FILE_EOF:
			if (Enum) *Enum = "FMOD_ERR_FILE_EOF";
			if (Message) *Message = "End of file unexpectedly reached while trying to read essential data (truncated data?).";
			break;
		case FMOD_ERR_FILE_NOTFOUND:
			if (Enum) *Enum = "FMOD_ERR_FILE_NOTFOUND";
			if (Message) *Message = "File not found.";
			break;
		case FMOD_ERR_FORMAT:
			if (Enum) *Enum = "FMOD_ERR_FORMAT";
			if (Message) *Message = "Unsupported file or audio format.";
			break;
		case FMOD_ERR_HTTP:
			if (Enum) *Enum = "FMOD_ERR_HTTP";
			if (Message) *Message = "A HTTP error occurred. This is a catch-all for HTTP errors not listed elsewhere.";
			break;
		case FMOD_ERR_HTTP_ACCESS:
			if (Enum) *Enum = "FMOD_ERR_HTTP_ACCESS";
			if (Message) *Message = "The specified resource requires authentication or is forbidden.";
			break;
		case FMOD_ERR_HTTP_PROXY_AUTH:
			if (Enum) *Enum = "FMOD_ERR_HTTP_PROXY_AUTH";
			if (Message) *Message = "Proxy authentication is required to access the specified resource.";
			break;
		case FMOD_ERR_HTTP_SERVER_ERROR:
			if (Enum) *Enum = "FMOD_ERR_HTTP_SERVER_ERROR";
			if (Message) *Message = "A HTTP server error occurred.";
			break;
		case FMOD_ERR_HTTP_TIMEOUT:
			if (Enum) *Enum = "FMOD_ERR_HTTP_TIMEOUT";
			if (Message) *Message = "The HTTP request timed out.";
			break;
		case FMOD_ERR_INITIALIZATION:
			if (Enum) *Enum = "FMOD_ERR_INITIALIZATION";
			if (Message) *Message = "FMOD was not initialized correctly to support this function.";
			break;
		case FMOD_ERR_INITIALIZED:
			if (Enum) *Enum = "FMOD_ERR_INITIALIZED";
			if (Message) *Message = "Cannot call this command after System::init.";
			break;
		case FMOD_ERR_INTERNAL:
			if (Enum) *Enum = "FMOD_ERR_INTERNAL";
			if (Message) *Message = "An error occured that wasnt supposed to. Contact support.";
			break;
		case FMOD_ERR_INVALID_HANDLE:
			if (Enum) *Enum = "FMOD_ERR_INVALID_HANDLE";
			if (Message) *Message = "An invalid object handle was used.";
			break;
		case FMOD_ERR_INVALID_PARAM:
			if (Enum) *Enum = "FMOD_ERR_INVALID_PARAM";
			if (Message) *Message = "An invalid parameter was passed to this function.";
			break;
		case FMOD_ERR_IRX:
			if (Enum) *Enum = "FMOD_ERR_IRX";
			if (Message) *Message = "PS2 only. fmodex.irx failed to initialize. This is most likely because you forgot to load it.";
			break;
		case FMOD_ERR_MEMORY:
			if (Enum) *Enum = "FMOD_ERR_MEMORY";
			if (Message) *Message = "Not enough memory or resources.";
			break;
		case FMOD_ERR_MEMORY_IOP:
			if (Enum) *Enum = "FMOD_ERR_MEMORY_IOP";
			if (Message) *Message = "PS2 only. Not enough memory or resources on PlayStation 2 IOP ram.";
			break;
		case FMOD_ERR_MEMORY_SRAM:
			if (Enum) *Enum = "FMOD_ERR_MEMORY_SRAM";
			if (Message) *Message = "Not enough memory or resources on console sound ram.";
			break;
		case FMOD_ERR_NEEDHARDWARE:
			if (Enum) *Enum = "FMOD_ERR_NEEDHARDWARE";
			if (Message) *Message = "Tried to use a feature that requires hardware support. (ie trying to play a VAG compressed sound in software on PS2).";
			break;
		case FMOD_ERR_NEEDSOFTWARE:
			if (Enum) *Enum = "FMOD_ERR_NEEDSOFTWARE";
			if (Message) *Message = "Tried to use a feature that requires the software engine. Software engine has either been turned off, or command was executed on a hardware channel which does not support this feature.";
			break;
		case FMOD_ERR_NET_CONNECT:
			if (Enum) *Enum = "FMOD_ERR_NET_CONNECT";
			if (Message) *Message = "Couldn't connect to the specified host.";
			break;
		case FMOD_ERR_NET_SOCKET_ERROR:
			if (Enum) *Enum = "FMOD_ERR_NET_SOCKET_ERROR";
			if (Message) *Message = "A socket error occurred. This is a catch-all for socket-related errors not listed elsewhere.";
			break;
		case FMOD_ERR_NET_URL:
			if (Enum) *Enum = "FMOD_ERR_NET_URL";
			if (Message) *Message = "The specified URL couldn't be resolved.";
			break;
		case FMOD_ERR_NOTREADY:
			if (Enum) *Enum = "FMOD_ERR_NOTREADY";
			if (Message) *Message = "Operation could not be performed because specified sound is not ready.";
			break;
		case FMOD_ERR_OUTPUT_ALLOCATED:
			if (Enum) *Enum = "FMOD_ERR_OUTPUT_ALLOCATED";
			if (Message) *Message = "Error initializing output device, but more specifically, the output device is already in use and cannot be reused.";
			break;
		case FMOD_ERR_OUTPUT_CREATEBUFFER:
			if (Enum) *Enum = "FMOD_ERR_OUTPUT_CREATEBUFFER";
			if (Message) *Message = "Error creating hardware sound buffer.";
			break;
		case FMOD_ERR_OUTPUT_DRIVERCALL:
			if (Enum) *Enum = "FMOD_ERR_OUTPUT_DRIVERCALL";
			if (Message) *Message = "A call to a standard soundcard driver failed, which could possibly mean a bug in the driver or resources were missing or exhausted.";
			break;
		case FMOD_ERR_OUTPUT_FORMAT:
			if (Enum) *Enum = "FMOD_ERR_OUTPUT_FORMAT";
			if (Message) *Message = "Soundcard does not support the minimum features needed for this soundsystem (16bit stereo output).";
			break;
		case FMOD_ERR_OUTPUT_INIT:
			if (Enum) *Enum = "FMOD_ERR_OUTPUT_INIT";
			if (Message) *Message = "Error initializing output device.";
			break;
		case FMOD_ERR_OUTPUT_NOHARDWARE:
			if (Enum) *Enum = "FMOD_ERR_OUTPUT_NOHARDWARE";
			if (Message) *Message = "FMOD_HARDWARE was specified but the sound card does not have the resources nescessary to play it.";
			break;
		case FMOD_ERR_OUTPUT_NOSOFTWARE:
			if (Enum) *Enum = "FMOD_ERR_OUTPUT_NOSOFTWARE";
			if (Message) *Message = "Attempted to create a software sound but no software channels were specified in System::init.";
			break;
		case FMOD_ERR_PAN:
			if (Enum) *Enum = "FMOD_ERR_PAN";
			if (Message) *Message = "Panning only works with mono or stereo sound sources.";
			break;
		case FMOD_ERR_PLUGIN:
			if (Enum) *Enum = "FMOD_ERR_PLUGIN";
			if (Message) *Message = "An unspecified error has been returned from a 3rd party plugin.";
			break;
		case FMOD_ERR_PLUGIN_MISSING:
			if (Enum) *Enum = "FMOD_ERR_PLUGIN_MISSING";
			if (Message) *Message = "A requested output, dsp unit type or codec was not available.";
			break;
		case FMOD_ERR_PLUGIN_RESOURCE:
			if (Enum) *Enum = "FMOD_ERR_PLUGIN_RESOURCE";
			if (Message) *Message = "A resource that the plugin requires cannot be found. (ie the DLS file for MIDI playback)";
			break;
		case FMOD_ERR_RECORD:
			if (Enum) *Enum = "FMOD_ERR_RECORD";
			if (Message) *Message = "An error occured trying to initialize the recording device.";
			break;
		case FMOD_ERR_REVERB_INSTANCE:
			if (Enum) *Enum = "FMOD_ERR_REVERB_INSTANCE";
			if (Message) *Message = "Specified Instance in FMOD_REVERB_PROPERTIES couldn't be set. Most likely because another application has locked the EAX4 FX slot.";
			break;
		case FMOD_ERR_SUBSOUND_ALLOCATED:
			if (Enum) *Enum = "FMOD_ERR_SUBSOUND_ALLOCATED";
			if (Message) *Message = "This subsound is already being used by another sound, you cannot have more than one parent to a sound. Null out the other parent's entry first.";
			break;
		case FMOD_ERR_TAGNOTFOUND:
			if (Enum) *Enum = "FMOD_ERR_TAGNOTFOUND";
			if (Message) *Message = "The specified tag could not be found or there are no tags.";
			break;
		case FMOD_ERR_TOOMANYCHANNELS:
			if (Enum) *Enum = "FMOD_ERR_TOOMANYCHANNELS";
			if (Message) *Message = "The sound created exceeds the allowable input channel count. This can be increased with System::setMaxInputChannels.";
			break;
		case FMOD_ERR_UNIMPLEMENTED:
			if (Enum) *Enum = "FMOD_ERR_UNIMPLEMENTED";
			if (Message) *Message = "Something in FMOD hasn't been implemented when it should be! contact support!";
			break;
		case FMOD_ERR_UNINITIALIZED:
			if (Enum) *Enum = "FMOD_ERR_UNINITIALIZED";
			if (Message) *Message = "This command failed because System::init or System::setDriver was not called.";
			break;
		case FMOD_ERR_UNSUPPORTED:
			if (Enum) *Enum = "FMOD_ERR_UNSUPPORTED";
			if (Message) *Message = "A command issued was not supported by this object. Possibly a plugin without certain callbacks specified.";
			break;
		case FMOD_ERR_UPDATE:
			if (Enum) *Enum = "FMOD_ERR_UPDATE";
			if (Message) *Message = "On PS2, System::update was called twice in a row when System::updateFinished must be called first.";
			break;
		case FMOD_ERR_VERSION:
			if (Enum) *Enum = "FMOD_ERR_VERSION";
			if (Message) *Message = "The version number of this file format is not supported.";
			break;
		default:
			if (Enum) *Enum = "(other)";
			if (Message) *Message = "Unknown error.";
			break;
		}
	}

	FmodError::FmodError(FMOD_RESULT code, const string &msg, const string &file, int line)
	{
		string e, m;
		CodeToMessage(code, &e, &m);
		Push("(FMOD,"+px::int2str((int)code)+","+e+") "+m);
		Push(msg, file, line);
	}
#endif

#ifdef WIN32
	Win32Error::Win32Error(const string &msg, const string &file, int line)
	{
		DWORD Code = GetLastError();
		string s = "(Win32Error," + UintToStrR<uint4>(Code) + ") ";
		char *Message = 0;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, Code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&Message, 0, 0);
		Push(s + Message);
		LocalFree(Message);

		Push(msg, file, line);
	}
#endif

#ifdef USE_DIRECTX
	DirectXError::DirectXError(HRESULT hr, const string &Msg, const string &File, int Line)
	{
		Push(Msg, File, Line);
	}

	DirectXError::DirectXError(HRESULT hr, ID3DXBuffer *Buf, const string &Msg, const string &File, int Line)
	{
		Push(Msg, File, Line);

		string M;
		if (Buf)
		{
			M.assign((const char*)Buf->GetBufferPointer(), Buf->GetBufferSize());
			Buf->Release();
			Push(M);
		}
	}
#endif

#ifdef USE_WINSOCK
	void WinSockError::CodeToStr(int Code, string *Name, string *Message)
	{
		Name->clear();
		Message->clear();

		switch (Code)
		{
		case WSAEINTR: *Name = "EINTR"; *Message = "Interrupted function call"; break;
		case WSAEACCES: *Name = "EACCES"; *Message = "Permission denied"; break;
		case WSAEFAULT: *Name = "EFAULT"; *Message = "Bad address"; break;
		case WSAEINVAL: *Name = "EINVAL"; *Message = "Invalid argument"; break;
		case WSAEMFILE: *Name = "EMFILE"; *Message = "Too many open files"; break;
		case WSAEWOULDBLOCK: *Name = "EWOULDBLOCK"; *Message = "Resource temporarily unavailable"; break;
		case WSAEINPROGRESS: *Name = "EINPROGRESS"; *Message = "Operation now in progress"; break;
		case WSAEALREADY: *Name = "EALREADY"; *Message = "Operation already in progress"; break;
		case WSAENOTSOCK: *Name = "ENOTSOCK"; *Message = "Socket operation on nonsocket"; break;
		case WSAEDESTADDRREQ: *Name = "EDESTADDRREQ"; *Message = "Destination address required"; break;
		case WSAEMSGSIZE: *Name = "EMSGSIZE"; *Message = "*Message too long"; break;
		case WSAEPROTOTYPE: *Name = "EPROTOTYPE"; *Message = "Protocol wrong type for socket"; break;
		case WSAENOPROTOOPT: *Name = "ENOPROTOOPT"; *Message = "Bad protocol option"; break;
		case WSAEPROTONOSUPPORT: *Name = "EPROTONOSUPPORT"; *Message = "Protocol not supported"; break;
		case WSAESOCKTNOSUPPORT: *Name = "ESOCKTNOSUPPORT"; *Message = "Socket type not supported"; break;
		case WSAEOPNOTSUPP: *Name = "EOPNOTSUPP"; *Message = "Operation not supported"; break;
		case WSAEPFNOSUPPORT: *Name = "EPFNOSUPPORT"; *Message = "Protocol family not supported"; break;
		case WSAEAFNOSUPPORT: *Name = "EAFNOSUPPORT"; *Message = "Address family not supported by protocol family"; break;
		case WSAEADDRINUSE: *Name = "EADDRINUSE"; *Message = "Address already in use"; break;
		case WSAEADDRNOTAVAIL: *Name = "EADDRNOTAVAIL"; *Message = "Cannot assign requested address"; break;
		case WSAENETDOWN: *Name = "ENETDOWN"; *Message = "Network is down"; break;
		case WSAENETUNREACH: *Name = "ENETUNREACH"; *Message = "Network is unreachable"; break;
		case WSAENETRESET: *Name = "ENETRESET"; *Message = "Network dropped connection on reset"; break;
		case WSAECONNABORTED: *Name = "ECONNABORTED"; *Message = "Software caused connection abort"; break;
		case WSAECONNRESET: *Name = "ECONNRESET"; *Message = "Connection reset by peer"; break;
		case WSAENOBUFS: *Name = "ENOBUFS"; *Message = "No buffer space available"; break;
		case WSAEISCONN: *Name = "EISCONN"; *Message = "Socket is already connected"; break;
		case WSAENOTCONN: *Name = "ENOTCONN"; *Message = "Socket is not connected"; break;
		case WSAESHUTDOWN: *Name = "ESHUTDOWN"; *Message = "Cannot send after socket shutdown"; break;
		case WSAETIMEDOUT: *Name = "ETIMEDOUT"; *Message = "Connection timed out"; break;
		case WSAECONNREFUSED: *Name = "ECONNREFUSED"; *Message = "Connection refused"; break;
		case WSAEHOSTDOWN: *Name = "EHOSTDOWN"; *Message = "Host is down"; break;
		case WSAEHOSTUNREACH: *Name = "EHOSTUNREACH"; *Message = "No route to host"; break;
		case WSAEPROCLIM: *Name = "EPROCLIM"; *Message = "Too many processes"; break;
		case WSASYSNOTREADY: *Name = "SYSNOTREADY"; *Message = "Network subsystem is unavailable"; break;
		case WSAVERNOTSUPPORTED: *Name = "VERNOTSUPPORTED"; *Message = "Winsock.dll version out of range"; break;
		case WSANOTINITIALISED: *Name = "NOTINITIALISED"; *Message = "Successful WSAStartup not yet performed"; break;
		case WSAEDISCON: *Name = "EDISCON"; *Message = "Graceful shutdown in progress"; break;
		case WSATYPE_NOT_FOUND: *Name = "TYPE_NOT_FOUND"; *Message = "Class type not found"; break;
		case WSAHOST_NOT_FOUND: *Name = "HOST_NOT_FOUND"; *Message = "Host not found"; break;
		case WSATRY_AGAIN: *Name = "TRY_AGAIN"; *Message = "Nonauthoritative host not found"; break;
		case WSANO_RECOVERY: *Name = "NO_RECOVERY"; *Message = "This is a nonrecoverable error"; break;
		case WSANO_DATA: *Name = "NO_DATA"; *Message = "Valid name, no data record of requested type"; break;
	//	case WSA_INVALID_HANDLE: *Name = "_INVALID_HANDLE"; *Message = "Specified event object handle is invalid"; break;
	//	case WSA_INVALID_PARAMETER: *Name = "_INVALID_PARAMETER"; *Message = "One or more parameters are invalid"; break;
	//	case WSA_IO_INCOMPLETE: *Name = "_IO_INCOMPLETE"; *Message = "Overlapped I/O event object not in signaled state"; break;
	//	case WSA_IO_PENDING: *Name = "_IO_PENDING"; *Message = "Overlapped operations will complete later"; break;
	//	case WSA_NOT_ENOUGH_MEMORY: *Name = "_NOT_ENOUGH_MEMORY"; *Message = "Insufficient memory available"; break;
	//	case WSA_OPERATION_ABORTED: *Name = "_OPERATION_ABORTED"; *Message = "Overlapped operation aborted"; break;
	//	case WSAINVALIDPROCTABLE: *Name = "INVALIDPROCTABLE"; *Message = "Invalid procedure table from service provider"; break;
	//	case WSAINVALIDPROVIDER: *Name = "INVALIDPROVIDER"; *Message = "Invalid service provider version number"; break;
	//	case WSAPROVIDERFAILEDINIT: *Name = "PROVIDERFAILEDINIT"; *Message = "Unable to initialize a service provider"; break;
		case WSASYSCALLFAILURE: *Name = "SYSCALLFAILURE"; *Message = "System call failure"; break;
		}

		if (!Name->empty()) *Name = "WSA" + *Name;
	}

	int WinSockError::WinSockError(int Code, const string &Msg, const string &File, int Line)
	{
		string Name, Text;
		CodeToStr(Code, &Name, &Text);
		Push( "(WinSockError,"+IntToStr(Code)+","+Name+") "+Text );
		Push(Msg, File, Line);
	}

	int WinSockError::WinSockError(const string &Msg, const string &File, int Line)
	{
		int Code = WSAGetLastError();
		string Name, Text;
		CodeToStr(Code, &Name, &Text);
		Push( "(WinSockError,"+IntToStr(Code)+","+Name+") "+Text );
		Push(Msg, File, Line);
	}
#endif

#ifdef USE_DEVIL
	DevILError::DevILError(const string &Msg, const string &File, int a_Line)
	{
		// Tu jest pêtla, bo w bibliotece DevILError jest stos b³êdów
		ILenum code;
		while ( (code = ilGetError()) != IL_NO_ERROR )
			Push( "(DevILError," + IntToStr(code) + ") " + iluErrorString(code) );
		Push(Msg, File, Line);
	}
#endif

#ifdef USE_AVI_FILE
	AVIFileError::AVIFileError(HRESULT hr, const string &Msg, const string &File, int Line)
	{
		string Message;
		switch (hr)
		{
		case AVIERR_OK             : Message = "AVIERR_OK"; break;
		case AVIERR_UNSUPPORTED	   : Message = "AVIERR_UNSUPPORTED"; break;
		case AVIERR_BADFORMAT	   : Message = "AVIERR_BADFORMAT"; break;
		case AVIERR_MEMORY		   : Message = "AVIERR_MEMORY"; break;
		case AVIERR_INTERNAL	   : Message = "AVIERR_INTERNAL"; break;
		case AVIERR_BADFLAGS	   : Message = "AVIERR_BADFLAGS"; break;
		case AVIERR_BADPARAM	   : Message = "AVIERR_BADPARAM"; break;
		case AVIERR_BADSIZE		   : Message = "AVIERR_BADSIZE"; break;
		case AVIERR_BADHANDLE	   : Message = "AVIERR_BADHANDLE"; break;
		case AVIERR_FILEREAD	   : Message = "AVIERR_FILEREAD"; break;
		case AVIERR_FILEWRITE	   : Message = "AVIERR_FILEWRITE"; break;
		case AVIERR_FILEOPEN	   : Message = "AVIERR_FILEOPEN"; break;
		case AVIERR_COMPRESSOR	   : Message = "AVIERR_COMPRESSOR"; break;
		case AVIERR_NOCOMPRESSOR   : Message = "AVIERR_NOCOMPRESSOR"; break;
		case AVIERR_READONLY	   : Message = "AVIERR_READONLY"; break;
		case AVIERR_NODATA		   : Message = "AVIERR_NODATA"; break;
		case AVIERR_BUFFERTOOSMALL : Message = "AVIERR_BUFFERTOOSMALL"; break;
		case AVIERR_CANTCOMPRESS   : Message = "AVIERR_CANTCOMPRESS"; break;
		case AVIERR_USERABORT	   : Message = "AVIERR_USERABORT"; break;
		case AVIERR_ERROR		   : Message = "AVIERR_ERROR"; break;
		default: Message = "Nieznany kod b³êdu"; break;
		}

		Push( "(AVIFileError," + IntToStr(hr) + ") " + Message);
		Push(Msg, File, Line);
	}
#endif

} // namespace common
