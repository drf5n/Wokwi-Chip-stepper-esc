# Wokwi-Chip-stepper-esc
## Description

stepper-esc -- Electronic Speed Controller for steppers.  Translates PWM to stepper quadrature

-  

To use this chip in your project, include it as a dependency in your `diagram.json` file:

```json
"dependencies": { "chip-stepper-esc": "github:drf5n/Wokwi-Chip-stepper-esc@1.0.1" }
```

Then, add the chip to your circuit by adding a `chip-stepper-esc` item to the `parts` section of `diagram.json`:

```json
  "parts": {
    ...,
    {
      "type": "chip-stepper-esc",
      "id": "esc1",
      "attrs": {"Tau":"10", "Texp":"-1", "MaxSpeed":"60" }
    },
```


The actual source code for the chip lives in [src/main.c](https://github.com/drf5n/Wokwi-Chip-stepper-esc/blob/main/src/main.c), and the pins are described in [chip.json](https://github.com/drf5n/Wokwi-Chip-stepper-esc/blob/main/chip.json).

## Examples

* [Wokwi Uno with L298N driver  using tabbed files](https://wokwi.com/projects/410302035690579969) -- with LED-motors and scopes
* [Wokwi Uno with L298N and stepper-ESCs  using tabs](https://wokwi.com/projects/410601389043609601) -- with LED motors and scopes


## Versions
* github:drf5n/Wokwi-Chip-stepper-esc@1.0.1 -- non-Working release


# notes on making a Wokwi custom chip work with Github repository dependency
To get the Wokwi build script working to build the necessary chip.zip file for distribution with a release so Wokwi can pick it up

1) enable the repository settings for workflow permissions to be read-write
2) make sure the .github/workflows/build.yaml is in the repository
3) commit
4) make a vN.n.n tag: `git tag -a "v1.0.5" -m "build.yaml"`
5) push the tag  to github: `git push origin tag v1.0.5`

Refer to https://discord.com/channels/787627282663211009/954892209486966825/1274569798231130163 for a little discussion 


## License

This project is licensed under the MIT license. See the [LICENSE](https://github.com/drf5na/Wokwi-Chip-L298N/blob/main/LICENSE) file for more details.
