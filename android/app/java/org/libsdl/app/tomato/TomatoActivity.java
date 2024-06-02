package org.libsdl.app.tomato;

import org.libsdl.app.SDLActivity;

public class TomatoActivity extends SDLActivity {
	protected String[] getLibraries() {
		return new String[] {
			// "SDL3", // we link statically
			// "SDL3_image",
			// "SDL3_mixer",
			// "SDL3_net",
			// "SDL3_ttf",
			// "main"
			"tomato"
		};
	}
}

