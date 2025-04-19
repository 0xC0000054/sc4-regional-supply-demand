# sc4-regional-supply-demand

A DLL Plugin for SimCity 4 that implements a basic regional supply/demand system.

The plugin can be downloaded from the Releases tab: https://github.com/0xC0000054/sc4-regional-supply-demand/releases

## Building Exemplar Properties

The DLL adds two new building exemplar properties that allow resources to be added or removed by plopping a building.
Both values use paired Uint32 items, consisting of a resource id and a quantity.

| Property Name | Property ID | Description |
|---------------|-------------|-------------|
| Regional Supply Consumed | 0x16F4C223 | The resources the building consumes from the regional supply. |
| Regional Supply Produced | 0x16F4C224 |The resources the building adds to the regional supply. |

## Lua Functions

The DLL provides a `regional_supply` table with the following functions for use by Lua code.

| Function Name | Description |
|---------------|-------------|
| add_to_demand | Adds to the existing demand for a resource. |
| remove_from_demand | Subtracts from the existing demand of a resource. |
| add_to_supply | Adds to the existing supply of a resource. |
| remove_from_supply | Subtracts from the existing supply of a resource. |
| get_resource_quantity | Gets the current quantity of a resource. |


## System Requirements

* Windows 10 or later

The plugin may work on Windows 7 or later with the [Microsoft Visual C++ 2022 x86 Redistribute](https://aka.ms/vs/17/release/vc_redist.x86.exe)
installed, but I do not have the ability to test that.

## Installation

1. Close SimCity 4.
2. Copy `SC4RegionalSupplyDemand.dll` and `RegionalSupplyDemand.dat` into the Plugins folder in the SimCity 4 installation directory.
3. Start SimCity 4.

## Troubleshooting

The plugin should write a `SC4RegionalSupplyDemand.log` file in the same folder as the plugin.    
The log contains status information for the most recent run of the plugin.

# License

This project is licensed under the terms of the GNU Lesser General Public License version 3.0.    
See [LICENSE.txt](LICENSE.txt) for more information.

## 3rd party code

[gzcom-dll](https://github.com/nsgomez/gzcom-dll/tree/master) Located in the vendor folder, MIT License.    
[Windows Implementation Library](https://github.com/microsoft/wil) - MIT License    

# Source Code

## Prerequisites

* Visual Studio 2022
* `git submodule update --init`

## Building the plugin

* Open the solution in the `src` folder
* Update the post build events to copy the build output to you SimCity 4 application plugins folder.
* Build the solution

## Debugging the plugin

Visual Studio can be configured to launch SimCity 4 on the Debugging page of the project properties.
I configured the debugger to launch the game in a window with the following command line:    
`-intro:off -CPUCount:1 -CPUPriority:high -w -CustomResolution:enabled -r1920x1080x32`

You may need to adjust the resolution for your screen.
