package org.srb2.nativecode;

import java.nio.ByteBuffer;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.view.SurfaceHolder;

public class Video {
	public static int width = 340;
	public static int height = 240;
	private SurfaceHolder sh;
	public ByteBuffer fb;
	public Bitmap bmp;

	public Video(SRB2 srb2, SurfaceHolder sh) {
		this.sh = sh;
		fb = ByteBuffer.allocateDirect(fbSize());
		bmp = Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565);
	}

	private int fbSize() {
		// naively assuming RGBA8888 now, even though that is entirely wrong.
		// ... well, at least, that's what the Canvas/Bitmap will expect.
		return width * height * 4;
	}

	public void gotFrame() {

		Canvas canvas = sh.lockCanvas();
		canvas.drawARGB(0xff, 0, 0, 0);
		// ugh, an extra copy. the only way to avoid this, I suppose,
		// is to use the surface in native code directly.
		bmp.copyPixelsFromBuffer(fb);
		canvas.drawBitmap(bmp, 0, 0, null);
		sh.unlockCanvasAndPost(canvas);

	}
}
