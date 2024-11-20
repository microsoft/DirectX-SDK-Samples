# Windows Firewall Install Helper

The `FirewallInstallHelper.dll` is a sample DLL that can be called from an installer to register an application with the Windows Firewall exception list.

See the [Windows Firewall for Game Developers](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/games-and-firewalls) article for more details.

The primary purpose is to avoid seeing the "Windows Security Alert" dialog.

## FirewallInstallHelper.dll

The support DLL exports the following functions:

* ``AddApplicationToExceptionListW`` - This function adds an application to the exception list. It takes a complete path to the executable and a friendly name that will appear in the firewall exception list. This function requires administrator privileges. 
* ``AddApplicationToExceptionListA`` - ANSI version of AddApplicationToExceptionListW 
* ``RemoveApplicationFromExceptionListW`` - This function removes the application from the exception list. It takes in a complete path to the executable. This function requires administrator privileges 
* ``RemoveApplicationFromExceptionListA`` - ANSI version of RemoveApplicationFromExceptionListW 
* ``CanLaunchMultiplayerGameW`` - This function reports if the application has been disabled or removed from the exceptions list. It should be called every time the game is run. The function takes in a complete path to the executable. This function does not require administrator privileges. 
* ``CanLaunchMultiplayerGameA`` - ANSI version of CanLaunchMultiplayerGameW 

And three functions to simplify integration with Windows Installer (MSI)
* ``SetMSIFirewallProperties`` 
* ``AddToExceptionListUsingMSI`` 
* ``RemoveFromExceptionListUsingMSI``

More Information
Games for Windows Technical Requirements (TR 1.1 and 1.2)
Games for Windows Test Cases (TR 1.1 and 1.2)
Where is the DirectX SDK (2021 Edition)?  
Games for Windows and DirectX SDK blog
Visual Studio 2012 Update 1

## More Information

[Games for Windows Technical Requirements (TR 1.1 and 1.2)](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/games-for-windows-technical-requirements-1-1-0006)   
[Games for Windows Test Cases (TR 1.1 and 1.2)](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/games-for-windows-test-requirements-1-0-0006)
