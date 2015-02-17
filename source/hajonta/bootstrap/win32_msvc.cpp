#pragma warning( disable : 4471 )
#pragma warning( disable : 4278 )

//The following #import imports EnvDTE based on its LIBID.
#import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("8.0") lcid("0") raw_interfaces_only named_guids
/*
//The following #import imports EnvDTE80 based on its LIBID.
#import "libid:1A31287A-4D7D-413e-8E32-3B374931BD89" version("8.0") lcid("0") raw_interfaces_only named_guids
//The following #import imports EnvDTE90 based on its LIBID.
#import "libid: 2ce2370e-d744-4936-a090-3fffe667b0e1" version("9.0") lcid("0") raw_interfaces_only named_guids
//The following #import imports EnvDTE90a based on its LIBID.
#import "libid: 64A96FE8-CCCF-4EDF-B341-FF7C528B60C9" version("9.0a") lcid("0") raw_interfaces_only named_guids
//The following #import imports EnvDTE100 based on its LIBID.
#import "libid: 26ad1324-4b7c-44bc-84f8-b86aed45729f" version("10.0") lcid("0") raw_interfaces_only named_guids
*/

int main()
{
    CoInitialize(0);

    CLSID classid = GUID_NULL;
    CLSIDFromProgID(L"VisualStudio.DTE", &classid);

    if(classid == GUID_NULL)
    {
        printf("Could not find CLSID for VisualStudio.DTE\n");
        exit(1);
    }

    EnvDTE::_DTE *dte = {};

    IUnknown *unknown;
    if (!SUCCEEDED(GetActiveObject(classid, 0, &unknown)))
    {
        printf("Failed to find running Visual Studio, exiting...\n");
        exit(1);
        /*
        printf("Could not get an active instance of Visual Studio, trying to start it\n");

        if (FAILED(CoCreateInstance(classid, NULL, CLSCTX_LOCAL_SERVER, EnvDTE::IID__DTE, (LPVOID *)&unknown)))
        {
            printf("Failed to start Visual Studio, exiting...\n");
            exit(1);
        }
        unknown->QueryInterface(&dte);
        printf ("Found instance of Visual Studio! - %p\n", dte);
        */
    }
    else
    {
        unknown->QueryInterface(&dte);
        printf ("Found instance of Visual Studio! - %p\n", dte);
    }

    EnvDTE::DebuggerPtr debugger;
    if (!SUCCEEDED(dte->get_Debugger(&debugger)))
    {
        printf("%p\n", debugger);
        printf("Could not get the Debugger from the interface\n");
        exit(1);
    }

    EnvDTE::dbgDebugMode mode;
    if (SUCCEEDED(debugger->get_CurrentMode(&mode)))
    {
        if (mode != EnvDTE::dbgDesignMode)
        {
            printf("Debugger is active.\n");
            EnvDTE::dbgEventReason reason;
            if (SUCCEEDED(debugger->get_LastBreakReason(&reason)))
            {
                switch (reason)
                {
case EnvDTE::dbgEventReasonAttachProgram: { printf("Attached to program\n"); } break;
case EnvDTE::dbgEventReasonBreakpoint: { printf("Breakpoint encountered\n"); } break;
case EnvDTE::dbgEventReasonContextSwitch: { printf("Switch in context.\n"); } break;
case EnvDTE::dbgEventReasonDetachProgram: { printf("Program detached.\n"); } break;
case EnvDTE::dbgEventReasonEndProgram: { printf("Program ended.\n"); } break;
case EnvDTE::dbgEventReasonExceptionNotHandled: { printf("Unhandled exception encountered.\n"); } break;
case EnvDTE::dbgEventReasonExceptionThrown: { printf("Exception thrown.\n"); } break;
case EnvDTE::dbgEventReasonGo: { printf("Execution started.\n"); } break;
case EnvDTE::dbgEventReasonLaunchProgram: { printf("Program launched.\n"); } break;
case EnvDTE::dbgEventReasonNone: { printf("No reason.\n"); } break;
case EnvDTE::dbgEventReasonStep: { printf("Execution step.\n"); } break;
case EnvDTE::dbgEventReasonStopDebugging: { printf("Debugging stopped.\n"); } break;
case EnvDTE::dbgEventReasonUserBreak: { printf("Execution interrupted by user.\n"); } break;
                    default: {
                        printf("Unknown\n");
                    } break;
                }
            }
        }
        else
        {
            printf("Debugger is not active.\n");
            debugger->Go(false);
        }
    }
}
