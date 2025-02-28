# Lon Lon MilkBar
## Lon Lon MilkBar is a fork of the original MilkBar Launcher project by the MilkBar Studios team. We DID NOT make the bulk of the mod, we just added features and made improvements.

You can find the changelog in this trello board: https://trello.com/b/VzEeQhBW/lon-lon-milkbar

The beta here means this *MIGHT have some features that dont fully work or are still in testing but have random errors

**There are four important folders in this project listed below:**

## [C#](C%23)
- This project contains the implementation for the server.
- The solution is located at `C#\GUIApp\GUIApp.sln`.
- This project is implemented on C# and uses sockets for the connectivity.

## [DLL](DLL/InjectDLL)
- This project contains the implementation for the code that is injected into the emulator.
- The solution is located at `DLL\InjectDLL\InjectDLL.sln`.
- The entrypoint for this project is located at `DLL\InjectDLL\dllmain.cpp`.
- Most of the implementation is based on finding important memory addresses to read and write the player information.
- This project is implemented on C++ and uses NamedPipes to communicate with the front end application.

## [WPF .NET 6](WPF%20.NET%206/Breath%20of%20the%20Wild%20Multiplayer)
- This projects contains the implementation for the front end application.
- The solution is located at `WPF .NET 6\Breath of the Wild Multiplayer\Breath of the Wild Multiplayer.sln`.
- This project is implemented in C# using WPF. This app works as an injector for our code and communicates with this code using NamedPipes.

## [BNP Files](BNP%20Files)
- This folder contains the bnp files that need to be installed in order to get the mod working.

# Building the project
Building the project should not be too complicated and this process can be automated using the [python script](buildWPF.py).
