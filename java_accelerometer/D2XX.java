class D2XX {

	public native int AccelerometerInit(int len, int freq);
	public native String AccelerometerRead();


	public int reg;
	public int val;
	public int dir;
	
    static {
        System.loadLibrary("D2XX");
        System.loadLibrary("ftd2xx");
    }

    public static void main(String[] args) {
        D2XX cfunc = new D2XX();
	cfunc.AccelerometerInit(30, 1);
	while(true)
	{
        System.out.println(cfunc.AccelerometerRead());
	}
    }
}

