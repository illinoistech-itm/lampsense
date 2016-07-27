# BSMP-2016-Intelligent-Lights
This repo contains the code and schematics for the Intelligent Lights Systems.

## Instructions for reconstructing the project
<ul>
	<li>Build the circuits according to the diagrams [Diagrams/Fritzing/].</li>
	<li>Configure the Philips Hue bridge and bulbs to work on the local network. You can add more lamps to the bridge by adding them and resetting the bridge. If that does not work, use the Philips Hue app for phones to add it manually using a specific ID that each lamp has.</li>
	<li>Configure the XBees modules: at least a coordinator, and a router or end device. Instructions for parameters are given on the report. Use the XCTU software for Windows, MAC or Linux to do so and a XBee USB Shield.</li>
	<li>Change the base code [Codes/base_arduino/base_arduino.ino] informing the correct IP and MAC addresses for the Ethernet shield and the Philips Hue bridge. You can find the address of the Philips Hue Bridge using its app for cellphones or by using the link: www.meethue.com/api/nupnp</li>
	<li>Upload the base code to Arduino Mega.</li>
	<li>Use a browser to open the Ethernet shield IP address configured on the base code.</li>
	<li>Use the buttons and sensors to operate the light bulbs.</li>
</ul>
## Additional comments
<ul>
	<li>You should be connected to the internet in order to watch the video on the presentation. It is hosted on YouTube.</li>
</ul>