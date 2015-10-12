// Code for sending GCode to a CNC machine via the serial port
// Written by Laurence Shann (laurence.shann@gmail.com)
//----------------------------------------------------------------------------------------
// This work is licensed under the Creative Commons Attribution 4.0 International License. 
// To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/ or 
// send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, 
// California, 94041, USA.
//----------------------------------------------------------------------------------------

using UnityEngine;
using System;
using System.Collections;
using System.IO;
using System.IO.Ports;
using System.Collections.Generic;
#if UNITY_EDITOR
using UnityEditor;
#endif


[ExecuteInEditMode]
public class CNCSend : MonoBehaviour {

    SerialPort port;
    static string response_success = "DONE";
    public bool openPort = false;
    public int portID = 2;
    public string[] ports;
   
     [Space(10)]
    public bool sendSingleCommand = false;
    public string singleCommand = "";
   
    [Space(10)]
    public bool sendFile = false;
    public string filename = "";
    
	public bool busy = false;

    float m_LastEditorUpdateTime;
    void OnEnable()
    {
        #if UNITY_EDITOR
        m_LastEditorUpdateTime = Time.realtimeSinceStartup;
        EditorApplication.update += OnEditorUpdate;
        #endif
        if (port != null) ClosePort();
        openPort = false;
		ports = null;
    }
    void OnEditorUpdate()
    {
        if (Time.realtimeSinceStartup > m_LastEditorUpdateTime+0.1f)
        {
            m_LastEditorUpdateTime = Time.realtimeSinceStartup;
            if (ports == null || ports.Length < 1)
            {
                GetPortNames();
            }

            if (portID > ports.Length)
                portID = 0;

            if (openPort && port == null) OpenPort();
            else if (!openPort && port != null) ClosePort();

            if (port != null && port.IsOpen) ProcessOpenPort();
            else openPort = false;
        }

    }


    void GetPortNames()
    {

        if (Environment.OSVersion.Platform == PlatformID.MacOSX || Environment.OSVersion.Platform == PlatformID.Unix)
        {
            ports = Directory.GetFiles("/dev/", "cu.*");
        }
        else
        {
            ports = SerialPort.GetPortNames();
            for (int a = 0; a < ports.Length; a++)
            {
                ports[a] = "\\\\.\\" + ports[a];
            }
        }
    }

    void OpenPort()
    {
        port = new SerialPort(ports[portID], 9600, Parity.None, 8, StopBits.One);
        port.ReadTimeout = 20;
        port.Handshake = Handshake.None;
        port.DtrEnable = true;
        port.RtsEnable = true;
        try { port.Open(); }
        catch { Debug.Log("failed to open port"); }

        if (port.IsOpen)
        {
            Debug.Log("ready for input");
        }
        else Debug.Log("port not open");
    }

    void ClosePort()
    {
        Debug.Log("closing port");
        port.Close();
        port = null;
    }

	public List<string> commandQueue;

	[TextArea(10,10)]
	public string received;


    void ProcessOpenPort()
    {
		if (commandQueue == null)
			commandQueue = new List<string> ();

        if (sendSingleCommand)
        {
			received="";
            sendSingleCommand = false;
			commandQueue.Add(singleCommand + "\r\n");
        }

        if (sendFile)
        {
			received="";
            sendFile = false;
            SendTextFile();
        }

		if (commandQueue.Count > 0 && !busy) {
			string next = commandQueue[0];
			commandQueue.RemoveAt(0);
			GCodeOp(next);
			busy=true;
		}

		int recLen = 0;
        try
        {
			do{
				recLen = received.Length;
            	received = received + (char)port.ReadChar();
			}while(recLen != received.Length);
		}
        catch (TimeoutException) {}
        
       
		if (received.Contains (response_success)) {

			if(received.Contains("DEBOUNCE") || received.Contains("{") || received.Contains("<")){
				Debug.LogWarning ("<color=yellow>"+received+"</color>");
			}else{
				Debug.Log (received);
			}
			received="";
			busy = false;
		}
	}


    void SendTextFile()
    {

        
        try
        {
            using (StreamReader sr = new StreamReader(Application.dataPath + "/" + filename))
            {
                string line = sr.ReadLine();

                while (line != null)
                {
                    commandQueue.Add(line + "\r\n");
                    line = sr.ReadLine();
                }
            }
        }
        catch (Exception e)
        {
            Debug.Log("The file could not be read:");
            Debug.Log(e.Message);
        }
    }

    void GCodeOp(string s)
    {
        //string received = "";
        port.WriteLine(s + "\r\n");
		/*
        do
        {
            try
            {
                received += (char)port.ReadChar();
            }
            catch (TimeoutException) {}
        } while (!received.Contains(response_success));
        Debug.Log("Data: "+received);
        Debug.Log("...done...");*/
    }

    
}
