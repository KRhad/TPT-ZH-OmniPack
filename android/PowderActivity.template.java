package @APPID@;

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
