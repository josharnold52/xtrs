
val raw = io.Source.fromFile("scantable.txt").getLines.map(_.trim).filter(_.nonEmpty).toVector

val Fmt = """^(.{20})\s+(\S+).*""".r

val table=for(Fmt(x,y) <- raw) yield (x.trim, y.trim)

val tMap = table.toMap

def entry(comment: String, i: Int, code: String, shift: String = "TK_Neutral", hasShift: Int = 0, c2: String = "TK_NULL", s2: String= "TK_Neutral") = {
    val f = "/* SCAN CODE 0x%02X       */    {{ %s, %s }, %s, {%s, %s}},  // %s"
    f.format(i, code, shift, hasShift, c2, s2, comment);
}

def regex(n: String): scala.util.matching.Regex = new scala.util.matching.Regex(n)


def lookup(pat: scala.util.matching.Regex): String = {
    val ms = table.iterator.filter(s => pat.pattern.matcher(s._1).find()).toVector
    require(ms.length == 1, f"$pat $ms")
    ms(0)._2
}

def addkey(name: String, pattern: String, code: String, shift: String = "TK_Neutral", hasShift: Int = 0, c2: String = "TK_NULL", s2: String= "TK_Neutral"): Unit = {
    val s = lookup(regex(pattern))
    println(s(0).toInt)
    println(s(1).toInt)
    val i = Integer.parseInt(s.replace('l','1'),16)
    outTable(i) = entry(name, i, code, shift, hasShift, c2, s2)
}


val outTable = Array.ofDim[String](128)

for(c <- 'a' to 'z') {
    val cu = c.toUpper
    val s = tMap(f"$c $cu")
    val i = Integer.parseInt(s,16)
    outTable(i) = entry(cu.toString, i, f"TK_$cu")
}


addkey("0 and )", "^0", "TK_0", "TK_ForceNoShift", 1, "TK_9", "TK_ForceShift")
addkey("1 and !", "^1", "TK_1")
addkey("2 and @", "^2", "TK_2", "TK_ForceNoShift", 1, "TK_AtSign", "TK_ForceNoShift")
addkey("3 and #", "^3", "TK_3")
addkey("4 and $", "^4", "TK_4")
addkey("5 and %", "^5", "TK_5")
addkey("6 and ^", "^6", "TK_6", "TK_ForceNoShift", 1)
addkey("7 and &", "^7", "TK_7", "TK_ForceNoShift", 1, "TK_6", "TK_ForceShift")
addkey("8 and *", "^8", "TK_8", "TK_ForceNoShift", 1, "TK_Colon", "TK_ForceShift")
addkey("9 and (", "^9", "TK_9", "TK_ForceNoShift", 1, "TK_8", "TK_ForceShift")
addkey("' and \"", "^'", "TK_7", "TK_ForceShift", 1, "TK_2", "TK_ForceShift")
addkey("; :", "^;", "TK_Semicolon", "TK_ForceNoShift", 1, "TK_Colon", "TK_ForceNoShift")
addkey("= +", "^=", "TK_Minus", "TK_ForceShift", 1, "TK_Semicolon", "TK_ForceShift")
addkey(", <", "^,", "TK_Comma")
addkey(". >", "^\\.", "TK_Period")
addkey("- _", "^\\- _", "TK_Minus", "TK_ForceNoShift", 1)
addkey("/ ?", "^/ \\?", "TK_Slash")


addkey("Enter", "^Enter$", "TK_Enter")
addkey("F12 (maps to clear)", "^F12", "TK_Clear")
addkey("Escape (maps to break)", "^Esc", "TK_Break")
addkey("Space", "^Space", "TK_Space")
addkey("Left Shift", "^Left Shift$", "TK_LeftShift")
addkey("Right Shift - forced left", "^Right Shift$", "TK_LeftShift")
addkey("Up Arrow", "^Up Arrow 8", "TK_Up")
addkey("Left Arrow", "^Left Arrow 4", "TK_Left")
addkey("Backspace (Map to Left)", "^Backspace", "TK_Left")
addkey("Right Arrow", "^Right Arrow 6", "TK_Right")
addkey("Down Arrow", "^Down Arrow 2", "TK_Down")
addkey("F2 (maps to shift-@)","^F1$","TK_AtSign","TK_ForceShift")

for(i <- 0 to 127) {
    if (outTable(i) == null) {
        outTable(i) = entry("???", i, "TK_NULL", "TK_Neutral")
    }
}

for(t <- outTable) {
    println(t)
}

val z = java.nio.file.Files.write(new java.io.File("generated_table.inc").toPath, outTable.mkString("\n").getBytes("US-ASCII"))

println(z)



