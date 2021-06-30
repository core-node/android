
package com.twosine.zsx;

/**********************************************
 * ZSX - java Web Client  
 * for android platform.   
 *
 *
 * http://zsx.mitchbux.repl.co/
 *
 * Mitchbux
 */
 
 
 
import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;
import android.webkit.*;
import java.io.*;
import android.os.*;

public class Client extends Activity
{
	
    @Override
    public void onCreate(Bundle savedInstanceState) 
	{
		final boolean is64bit = Build.SUPPORTED_64_BIT_ABIS.length > 0;
		
        super.onCreate(savedInstanceState);
		
		WebView webView = new WebView(this);
		setContentView(webView);
		webView.getSettings().setJavaScriptEnabled(true);
        webView.getSettings().setLoadWithOverviewMode(true);
        webView.getSettings().setUseWideViewPort(true);
        webView.getSettings().setSupportZoom(true);
        webView.getSettings().setBuiltInZoomControls(true);
        webView.getSettings().setDisplayZoomControls(false);
        webView.setScrollBarStyle(WebView.SCROLLBARS_OUTSIDE_OVERLAY);
        setDesktopMode(webView, true);
		
		new Thread(new Runnable() {public void run(){try{
			while (!Thread.currentThread().isInterrupted()) 
				serveHTTP(); 
			}catch (Exception e){e.printStackTrace();}}}).start();
			
		webView.loadUrl("http://localhost:7337/");
	}

	
	public void setDesktopMode(WebView webView,boolean enabled) {
		String newUserAgent = webView.getSettings().getUserAgentString();
		if (enabled) {
			try {
				String ua = webView.getSettings().getUserAgentString();
				String androidOSString = webView.getSettings().getUserAgentString().substring(ua.indexOf("("), ua.indexOf(")") + 1);
				newUserAgent = webView.getSettings().getUserAgentString().replace(androidOSString, "(X11; Linux x86_64)");
			} catch (Exception e) {
				e.printStackTrace();
			}
		} else {
			newUserAgent = null;
		}
		webView.getSettings().setUserAgentString(newUserAgent);
		webView.getSettings().setUseWideViewPort(enabled);
		webView.getSettings().setLoadWithOverviewMode(enabled);
		webView.reload();
	}
	
    public native String  serveHTTP();
    static {
        System.loadLibrary("zsx");
    }
}






/*																	†
  http://zsx.mitchbux.repl.co/										†
 																	†
  Mitchbux 															†
 								{ 2021 } 							†
 *******************************************************************/
 
