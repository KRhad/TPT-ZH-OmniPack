package @APPID@;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import android.widget.Toast;
import org.libsdl.app.SDLActivity;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.util.Enumeration;
import java.io.IOException;
import java.io.File;
import java.io.FileInputStream;
import java.io.OutputStream;
import android.util.Base64;

public class PowderActivity extends SDLActivity
{
	private String pendingUpdateFile;
	private boolean waitingForInstallPermission;

	public void openUri(final String uri)
	{
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				try {
					Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
					startActivity(intent);
				} catch (RuntimeException error) {
					Toast.makeText(PowderActivity.this, "Unable to open link", Toast.LENGTH_LONG).show();
				}
			}
		});
	}

	public void installApkUpdate(final String filename)
	{
		runOnUiThread(new Runnable()
		{
			@Override
			public void run()
			{
				pendingUpdateFile = filename;
				continueApkUpdate();
			}
		});
	}

	private void continueApkUpdate()
	{
		if (pendingUpdateFile == null) {
			return;
		}
		if (Build.VERSION.SDK_INT >= 26 && !getPackageManager().canRequestPackageInstalls()) {
			waitingForInstallPermission = true;
			try {
				Intent permission = new Intent(
					Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
					Uri.parse("package:" + getPackageName()));
				startActivity(permission);
			} catch (RuntimeException error) {
				waitingForInstallPermission = false;
				pendingUpdateFile = null;
				Toast.makeText(this, "Unable to open the Android install permission screen", Toast.LENGTH_LONG).show();
			}
			return;
		}
		final File apk = new File(pendingUpdateFile);
		pendingUpdateFile = null;
		waitingForInstallPermission = false;
		new Thread(new Runnable()
		{
			@Override
			public void run()
			{
				stageApkUpdate(apk);
			}
		}, "OmniPack APK installer").start();
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		if (waitingForInstallPermission && pendingUpdateFile != null) {
			waitingForInstallPermission = false;
			if (Build.VERSION.SDK_INT < 26 || getPackageManager().canRequestPackageInstalls()) {
				continueApkUpdate();
			} else {
				pendingUpdateFile = null;
				Toast.makeText(this, "Install permission was not granted; the verified APK was not installed", Toast.LENGTH_LONG).show();
			}
		}
	}

	private void stageApkUpdate(final File apk)
	{
		PackageInstaller.Session session = null;
		try {
			if (!apk.isFile() || apk.length() == 0) {
				throw new IOException("verified APK is missing");
			}
			PackageInstaller installer = getPackageManager().getPackageInstaller();
			PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
				PackageInstaller.SessionParams.MODE_FULL_INSTALL);
			params.setAppPackageName(getPackageName());
			if (Build.VERSION.SDK_INT >= 26) {
				params.setInstallReason(PackageManager.INSTALL_REASON_USER);
			}
			int sessionId = installer.createSession(params);
			session = installer.openSession(sessionId);
			try (FileInputStream input = new FileInputStream(apk);
				 OutputStream output = session.openWrite("TPT-ZH-OmniPack.apk", 0, apk.length())) {
				byte[] buffer = new byte[1024 * 1024];
				int count;
				while ((count = input.read(buffer)) != -1) {
					output.write(buffer, 0, count);
				}
				session.fsync(output);
			}

			Intent statusIntent = new Intent(this, UpdateInstallReceiver.class);
			statusIntent.setAction(getPackageName() + ".UPDATE_INSTALL_STATUS");
			int flags = PendingIntent.FLAG_UPDATE_CURRENT;
			if (Build.VERSION.SDK_INT >= 31) {
				flags |= PendingIntent.FLAG_MUTABLE;
			}
			PendingIntent status = PendingIntent.getBroadcast(this, sessionId, statusIntent, flags);
			session.commit(status.getIntentSender());
		} catch (final Exception error) {
			runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					Toast.makeText(PowderActivity.this, "Unable to stage update: " + error.getMessage(), Toast.LENGTH_LONG).show();
				}
			});
		} finally {
			if (session != null) {
				session.close();
			}
			apk.delete();
		}
	}

	public static class UpdateInstallReceiver extends BroadcastReceiver
	{
		@Override
		public void onReceive(Context context, Intent intent)
		{
			int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
			if (status == PackageInstaller.STATUS_PENDING_USER_ACTION) {
				Intent confirmation = intent.getParcelableExtra(Intent.EXTRA_INTENT);
				if (confirmation != null) {
					confirmation.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					context.startActivity(confirmation);
				}
			} else if (status != PackageInstaller.STATUS_SUCCESS) {
				String detail = intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE);
				Toast.makeText(context, "Update installation failed: " + detail, Toast.LENGTH_LONG).show();
			}
		}
	}

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
