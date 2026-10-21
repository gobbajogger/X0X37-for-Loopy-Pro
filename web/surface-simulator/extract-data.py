"""Read-only extraction of committed firmware tables; no SDK or captures."""
import pathlib,re,json
root=pathlib.Path(__file__).resolve().parents[2]/'firmware/src/app'
out={}
for key,file,symbol,kind in [('pad','apc40_pad_owner.h','APC40_PAD_OWNER','rgb'),('lcd','apc40_lcd565.h','APC40_LCD565','hex'),('idle','apc40_lcd_idle.h','APC40_LCD_IDLE565','hex'),('scales','smk_clip_keys.h','CK_SCALE_MASK','hex'),('chords','smk_clip_keys.h','CK_CHORD_T','hex')]:
 s=(root/file).read_text(); body=s.split(symbol,1)[1].split('=',1)[1].split(';',1)[0]
 out[key]=[list(map(int,x.split(','))) for x in re.findall(r'\{([0-9,]+)\}',body)] if kind=='rgb' else [int(x,16) for x in re.findall(r'0x[0-9A-Fa-f]+',body)]
pathlib.Path(__file__).with_name('data.js').write_text('/* Extracted from firmware 8145957. GPL-3.0-only. */\nconst FirmwareData='+json.dumps(out,separators=(',',':'))+';\nif(typeof module!=="undefined") module.exports=FirmwareData;\n')
