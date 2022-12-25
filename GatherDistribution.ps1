$libs = "Neo-Steelgear-Graphics-RenderQueue64D.lib", "Neo-Steelgear-Graphics-RenderQueue64.lib"

Get-ChildItem -Path "NSGG RenderQueue" -Include *.* -File -Recurse | foreach { $_.Delete()}

foreach ($file in $libs)
{
	

	$path = Get-ChildItem -Path "" -Filter $file -Recurse -ErrorAction SilentlyContinue -Force
	if ($path)
	{
		Copy-Item $path.FullName -Destination "NSGG RenderQueue\\Libraries\\"
	}
	else
	{
		Write-Host "Could not find file:" $file
	}
}

Get-ChildItem -Path ".\Neo-Steelgear-Graphics-RenderQueue" -Filter *.h* | foreach { Copy-Item $_.FullName -Destination "NSGG RenderQueue\\Headers\\"}