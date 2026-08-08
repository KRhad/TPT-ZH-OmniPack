package @APPID@;

import android.app.PendingIntent;
import android.content.Intent;
import android.os.Build;
import org.libsdl.app.SDLActivity;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.util.Enumeration;
import java.io.IOException;
import java.io.File;
import android.util.Base64;

public class PowderActivity extends SDLActivity
{
	public void restartApplication()
	{
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				Intent restartIntent = Intent.makeRestartActivityTask(getComponentName());
				int flags = PendingIntent.FLAG_CANCEL_CURRENT | PendingIntent.FLAG_ONE_SHOT;
				if (Build.VERSION.SDK_INT >= 23) {
					flags |= PendingIntent.FLAG_IMMUTABLE;
				}
				PendingIntent restart = PendingIntent.getActivity(
					PowderActivity.this, 0x545054, restartIntent, flags);
				try {
					restart.send();
				} catch (PendingIntent.CanceledException e) {
					startActivity(restartIntent);
				}
				finishAffinity();
				android.os.Process.killProcess(android.os.Process.myPid());
			}
		});
	}

	public String getCertificateBundle()
	{
		String allPems = "";
		try {
			KeyStore ks = KeyStore.getInstance("AndroidCAStore");
			if (ks != null) {
				ks.load(null, null);
				Enumeration<String> aliases = ks.aliases();
				while (aliases.hasMoreElements()) {
					String alias = (String)aliases.nextElement();
					java.security.cert.X509Certificate cert = (java.security.cert.X509Certificate)ks.getCertificate(alias);
					allPems += "-----BEGIN CERTIFICATE-----\n" + Base64.encodeToString(cert.getEncoded(), Base64.NO_WRAP) + "\n-----END CERTIFICATE-----\n";
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
			return "";
		} catch (KeyStoreException e) {
			e.printStackTrace();
			return "";
		} catch (NoSuchAlgorithmException e) {
			e.printStackTrace();
			return "";
		} catch (java.security.cert.CertificateException e) {
			e.printStackTrace();
			return "";
		}
		return allPems;
	}

	public String getDefaultDdir()
	{
		File dataDir = getExternalFilesDir(null);
		if (dataDir == null) {
			dataDir = getFilesDir();
		}
		return dataDir.getAbsolutePath();
	}
}
