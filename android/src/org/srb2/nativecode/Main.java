package org.srb2.nativecode;

import java.nio.ByteBuffer;

public class Main {
	private SRB2 srb2;

	public Main(SRB2 srb2) {
		this.srb2 = srb2;
	}

	public native int main(Video v);
}
