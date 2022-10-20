$Project
{
	$Folder "Protobuf Files"
	{
		$File "$SRCDIR/game/shared/econ/econ_messages.proto"
		$Folder "Generated Files" [!$OSXALL]
		{
			$DynamicFile "$GENERATED_PROTO_DIR/econ_messages.pb.h"
			$DynamicFile "$GENERATED_PROTO_DIR/econ_messages.pb.cc"
			{
				$Configuration
				{
					$Compiler 
					{
						$Create/UsePrecompiledHeader	"Not Using Precompiled Headers"
						$AdditionalOptions				"$BASE /D_HAS_EXCEPTIONS=0"
					}
				}
			}
		}
	}
}
