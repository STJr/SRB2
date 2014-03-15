<?php
header("Content-type: text/html; charset=ISO-8859-1");
ob_start("ob_gzhandler");
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<HTML>
<HEAD>
<TITLE>SRB2 Master Server Status</TITLE>
</HEAD>
<BODY BGCOLOR="#000000" TEXT="#FFFFFF">
<CENTER>
  <TABLE BORDER="0" WIDTH="835">
    <TR>
      <TD WIDTH="444">
        <CENTER>
          <A HREF="http://www.srb2.org/"> <IMG SRC="http://clipper.ship.edu/~af0916/srb2/corner.png" ALT="SRB2 logo." WIDTH="120" HEIGHT="84">
          <BR>
          SRB2's HomePage</A>
        </CENTER>
      </TD>
      <TD WIDTH="392">
        <CENTER>
          <A HREF="http://chompo.home.comcast.net/"><IMG SRC="http://chompo.home.comcast.net/cwimages/topbar.jpg" ALT="Chompy World logo." WIDTH="386" HEIGHT="64">
          <BR>
          Chompy World</A>
        </CENTER>
      </TD>
    </TR>
    <TR>
      <TD WIDTH="444">
        <CENTER>
          <A HREF="http://www.sepwich.com/foxboy/Srb2World/"><IMG SRC="http://www.sepwich.com/foxboy/Srb2World/title.jpg" ALT="SRB2 World logo." WIDTH="436" HEIGHT="52">
          <BR>
          SRB2 World</A>
        </CENTER>
      </TD>
      <TD WIDTH="389">
        <!-- <CENTER>
          <A HREF="http://jte.rose-manor.net/"><img src="http://jte.rose-manor.net/images/logo.png" alt="JTE Website logo." width="295" height="43">
          <BR>
          Jason the Echidna's website</A>
        </CENTER> !-->
      </TD>
    </TR>
  </TABLE>
</CENTER>
<HR>

<?php
	// ob_start("mb_output_handler");
        $host_addr = "Alam_GBC's box (srb2.servegame.org : 28900)";
        $fd = fsockopen("srb2.servegame.org", 28900, $errno, $errstr, 5);
	if ($fd)
	{
		echo "<h3>SRB2 Master Server Status</h3>\nCurrent host: $host_addr<br>";
		$buff = "000043210000";
		fwrite($fd, $buff);
		while (1)
		{
			$content=fgets($fd, 13); // skip 13 first bytes
			$content=fgets($fd, 1024);
			echo "$content";
			if (feof($fd)) break;
		}
		fclose($fd);
		
	}
	else
	{
		echo 'The master server is not running <br>';
		echo "ERROR: $errno - $errstr<br />\n";
	}
?>
<HR>
<H2>Info:</H2>
<A HREF="http://fanmade.emulationzone.org/gregorsoft/launcher.htm">Win32 SRB2MSLauncher</A> Great job, "Gregor Dick"/Oogaland of <A HREF="http://fanmade.emulationzone.org/gregorsoft/">Gregorsoft Software</A>
<BR>
<HR><BR>
<H2>Version Info:</H2>
srb2dos.exe (DOS/Allegro/WATTCP32 version)<BR>
srb2.exe    (Windows/DirectX/FMOD version)<BR>
srb2sdl.exe (Windows/SDL/SDL_mixer version)<BR>
lsdlsrb2    (GNU/Linux/SDL/SDL_mixer version)<BR>
<BR>
<A HREF="http://clipper.ship.edu/~af0916/srb2/download.htm">Srb2win.exe</A> v1.09.4 (use Internet search menu)<BR>
</BODY>
</HTML>
<?php
ob_flush();
?>
