# Visual Novel Interpreter
A Visual Novel engine inspired by Ren'Py. Games are made using a custom script language.

Personal project written in C to discover low-level programming, game engine programming, OpenGL, language lexing-parsing and design.

![](https://cdn.discordapp.com/attachments/522499136449413123/732652083831898132/VNI-pres.gif)  
*A "999: Nine Hours, Nine Persons, Nine Doors" fan game I'm doing to test my engine.*

This scene is created by this script:
```
#set_window_name "999 fan-made game"
#set_background "First class cabin"::"piano"

>"Junpei" left
	We've finally completed this puzzle.

>"Seven" right "thinking" center
	I still don't understand the purpose of this killing game.
```
## Status
This is a personal project I work on during my free time, I update it from time to time.  
It is in early development, only basic features are available, and the syntax may change over time, but you can still create a simple Visual Novel game with it, and I have some ideas for future features and improvements.  
However I keep it functional, so if you're experiencing bugs, unwanted behaviours or having any question please contact me !
## Build and use
**Windows - MinGW**

There is no dependencies, so just clone the repository and run `build.bat` to compile the project.  
You can then launch the game with `VisualNovelInterpreter.exe`.  
`Dialogs/start.dlg` is the game starting point and is editable with any text editor.  
## Language features
### Dialog
#### Basic dialog
To make someone talk the syntax is the following:
```
>"Junpei" left
	We've finally completed this puzzle.
```
The speaker's name is introduced by the `>` symbol and demarcated by `"`.  
The `left` keyword specifies the speaker's name display location. It can be displayed either at the `left` or the `right` of the screen.  
The position is mandatory.

<img src="https://cdn.discordapp.com/attachments/522499136449413123/732674822567362640/unknown.png" width="480" height="360"/>

It is also possible to display text without any speaker name, like with a narrator:
```
>
	Snake didn't answered, maybe he didn't heard Junpei at all.
```
<img src="https://cdn.discordapp.com/attachments/522499136449413123/733444343595139202/unknown.png" width="480" height="360"/>

Successives sentences can be assigned to the same character:
```
>"Junpei" left
	I'll ask you in case.
	Thank you Seven.
```
Tabulations are part of the language, as in Python.  
Sentences must be indented one level further than the corresponding speaker declaration.  
*The speaker declaration and the following indented sentences form a **cue**.*

<img src="https://cdn.discordapp.com/attachments/522499136449413123/733445975737106532/VNI_-_multiline_no_anim.gif" width="480" height="360"/>

---
#### Animated dialog
To display a character, you need to specify its **animation** and its position on the screen:
```
>"Seven" right "thinking" center
	I still don't understand the purpose of this killing game.
```
<img src="https://cdn.discordapp.com/attachments/522499136449413123/732688634712686722/VNI_-_animated_dialog.gif" width="480" height="360"/>


Seven's name is displayed at the right of the screen.  
Its sprite is drawn on the center of the screen, with its "thinking" **animation** playing while he's talking.

There are 7 drawing locations for the character sprites: `full-left`, `left`, `center-left`,`center`,`center-right`,`right` and `full-right`.  
We'll see later how to add character **animations** to the game.

---
### Choices
This sequence contains a **choice**:
```
>"Seven" right "thinking" right
	Then we should've missed something in this room that will permit us to open this safe.

>"Snake" left
	Or maybe we'll found the way to open that safe later.

>
	Junpei wanted to..
	-Search this room again.
		->search_again
	-Get out of here to meet the others.
		->get_out
```
<img src="https://cdn.discordapp.com/attachments/506035655206502410/731573090018132089/VNI-choice.gif" width="480" height="360"/>

A **choice option** is declared in a **cue** as normal sentences, prepended by a `-`.  
It must be followed on the next line by a **go-to** operator `->` with its **knot** destination, which are both indented one level further than the **choice option**.  
***Knots** and **go-tos** are explained just below.*  
Once a **cue** contains a **choice**, only other **choice options** can follow.  

---
### Flow
A way to control the flow of your script is to use **knots** and **go-tos**.

A **knot** is a named part of the script, the user can declare one with the `@` symbol:
```
@my_knot
```
A **knot** must be declared out of any **cue** or **condition**, since it demarcates a part of the script.

You can go to a **knot** by using the **go-to** operator `->` anywhere in the script.
```
@my_knot			// the declaration out of any cue or condition

->my_knot			// inside a knot

>
	->my_knot		// inside a cue

>
	-choice
		->my_knot	// inside a cue, as a choice destination

#if myVar
	->my_knot		// inside a condition
```
A **knot** named `start` is implicitly declared at the top of any script file, which means that a user-declared **knot** cannot be named `start`.

It is also possible to access **knots** from other script files using the following syntax:
```
->"file.ext"::my_knot
```
If the interpreter comes to the end of a **knot** without encountering any **go-tos**, it will continue into the next.
```
@first_knot

>
	This sentence is said first.

@second_knot

>
	This sentence is said secondly.
```
When the interpreter comes to the end of a script file, the game closes.

---
### Conditions and variables
As you may have already seen, it is possible to control the script's flow by **conditions**.

#### Conditions
A **condition** can be used with the following syntax:
```
#if sevenTrustsJunpei
	->reveal_scene
#else
	->end_of_investigation
```
When encountering a **condition**, the interpreter evaluates the expression following the `#if` keyword, and then goes into the corresponding **branch**. The `#else` **branch** is optional.  
A **branch** is a group of **cues**, **commands**, **choice options**, **go-tos**, or even another **conditions** that are indented one level further their corresponding **condition**.

**Conditions** can be used from a **knot**, inside a **cue** or another **condition**.

The syntax for conditional **choice options** is the following:
```
>
	-Fist choice
		->first_outcome
	-Second choice
		->second_outcome
	#if knowsAboutNinthBracelet
		-Third choice
			->third_outcome
```
#### Variables
Variables have *an identifier* and *a value*.  
The identifier is the keyword you will be refering to when using the variable.  
The value is either a numeric value, or a string value.  
There are `true` and `false` keywords, but they are just aliases to respectively `1` and `0`.  

You can assign a value to a variable with the following syntax:
```
#assign keyPartsFound 5

#assign murder'sName "unknown"
```
You can perform some operations on variables:
```
assign canPassDoor hasKey && (foundClues + foundPapers > 3)
```
The available operators list is: `(`, `)`, `&&`, `||`, `!`, `==`, `!=`, `+`, `-`, `*`, `/`, `<`, `<=`, `>`, `>=`.

You can't perform operations between string variables and numeric variables.

---
### Commands
**Commands** are ways to interact with the engine, like playing music, display sprites, adjust timing etc.  
The syntax of a **command** is the following:
```
#command_name
#another_command_name argument1 argument2
```
#### Animations commands
<img src="https://cdn.discordapp.com/attachments/506035655206502410/731573091314040872/VNI-backgrounds.gif" width="480" height="360"/>

You can set the background sprite with the following **command**:
```
#set_background "First class cabin"::"piano"
```
It will search the **animation** "piano" of background **pack** "First class cabin".  
This means you can have an animated background!  
The fade transition is automatic.

To set a character **animation** that will be played while talking, use the following **command**:
```
#set_character center "Snake"::"thinking"
```
It will display at the center of the screen the first sprite of the "thinking" **animation** of the "Snake" character **animation pack**.  
The **animation** will be played when "Snake" will speak.

The dialog syntax:
```
>"Snake" center "thinking" center
	Hmm..
```
is equivalent to:
```
#set_character center "Snake"::"thinking"
>"Snake" center
	Hmm..
```

#### Animation files
To use an **animation** in a script, you must create a descriptive **animation** file.  
It describes a character **animation pack** or a background **pack**.  
Those files are placed in the "Animation files" folder, and are named "**pack**.anm".

Here is an example:
```
// Seven.anm

"thinking" loop
	"thinking1.gif" 0.1 0.6 0.75
	"thinking2.gif" 0.1 0.6 0.75
	"thinking3.gif" 0.1 0.6 0.75
	"thinking2.gif" 0.1 0.6 0.75

"laughing" loop
	"laughing1.gif" 0.1 0.6 0.75
	"laughing2.gif" 0.1 0.6 0.75
	"laughing3.gif" 0.1 0.6 0.75
	"laughing2.gif" 0.1 0.6 0.75
```
This file describes 2 **animations** of the "Seven" character, they are composed of **animation frames**, which are indented one level further than their corresponding animation.  
The identifier `loop` specifies that these are **looped animations**.

The syntax of an **animation frame** is the following:
```
"name of frame file.ext" duration screenWidth screenHeight
```
Duration is in seconds, width and height are between `0` and `1` and are screen proportions.  
If the dimensions aren't specified, the pixel dimensions will be used, but it won't be responsive.

There is another type of **animations**:
```
// First class cabin.anm

"piano" static
	"piano.png"
```
**Static animations** are marked by the `static` keyword, and composed of only one **animation frame**.  
The duration cannot be specified (it would have no sense).  
The frame dimensions aren't specified, but since it's a background, it will be adjusted to cover all the screen.

#### Other commands
`#clear_background` clears background, revealing the window's clear color.

`#clear_character_position position` removes the character at the specified position.

`#clear_character_positions` removes all characters on the screen.

`#play_music "music_name.ext` plays a music.
There is only one music at a time, like backgounds, an automatic cross fade is made.

`#stop_music` stops the playing music.

`#set_music_volume volume` changes the music volume, volume's value is between `0` and `1`.

`#play_sound "sound_name.ext` plays a sound.
There is only one sound at a time, like backgounds, an automatic cross fade is made.

`#stop_sound` stops the playing sound.

`#set_sound_volume volume` changes the sound volume, volume's value is between `0` and `1`.

`#hide_ui` hides UI until it has to be displayed again.

`#wait duration` make the script wait, duration is in seconds.

`#set_window_name "new window name"` changes the window name.

`#set_speaker_name_color "speaker name" red green blue` changes the display color of the corresponding speaker's name, color arguments are between `0` and `1`.
## Engine features
### Window
Window with OpenGL 3.3 context using Win32 API.  
3 window modes: Fullscreen - Borderless - Windowed.  
Dynamic resize, focus-aware, make these informations accessibles in gameplay code.

#### Example: responsivity
<img src="https://cdn.discordapp.com/attachments/506035655206502410/731573086876336137/VNI-resize.gif" width="480" height="360"/>

---
### Graphics
Displays static and animated sprites.  
Images read thanks to [stb_image](https://github.com/nothings/stb).

Displays Unicode text with TrueType fonts.  
Text can have a display width limit, so it can dynamically adjust himself.  
Fonts read thanks to [stb_truetype](https://github.com/nothings/stb).

<img src="https://cdn.discordapp.com/attachments/506035655206502410/731573091314040872/VNI-backgrounds.gif" width="480" height="360"/>

---
### User input
Mouse, keyboard, scroll wheel, mouse side buttons input with Win32 API.  
Accessible in gameplay code.

<img src="https://cdn.discordapp.com/attachments/522499136449413123/733432898388099192/VNI_-_input.gif" width="480" height="360"/>

---
### Hot reload
When pressing the `R` key, the game is reloaded, taking into account saved changes in dialog script.

<img src="https://cdn.discordapp.com/attachments/522499136449413123/733492691186352168/VNI_-_hot_reload_light.gif" width="960" height="360"/>

---
### Audio
Plays, pauses, stops sounds and musics at wanted volume.  
Reads audio assets thanks to [miniaudio](https://github.com/dr-soft/miniaudio).

---
### Others
#### Error and warning windows
```
>
	Junpei wanted to..
	-Search this room again.
		->search_again
	-Get out of here to meet the others.
			->get_out 					// too many tabulations
```
produces

<img src="https://cdn.discordapp.com/attachments/522499136449413123/733427607491706991/unknown.png" width="300" height="120"/>

---
#### UTF-8 and UTF-16 handling
<img src="https://cdn.discordapp.com/attachments/522499136449413123/733067648442564739/unknown.png" width="480" height="360"/>

---
#### Memory leak tracking.
```c
int *integer = xmalloc(sizeof (*integer));
// xfree(integer);

print_leaks();
```
produces
```
--- Memory leaks ---
        -main.c:148 &205199872
1 memory leaks.
```