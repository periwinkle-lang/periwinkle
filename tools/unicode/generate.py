from dataclasses import dataclass, astuple
from enum import Enum, IntEnum, auto
from contextlib import redirect_stdout
from urllib import request
from io import StringIO
from collections import Counter
from sys import maxsize

# Вихідні назви файлів
UNICODE_DATABASE_HPP = "unicode_database.hpp"

UNICODE_REPO = "https://www.unicode.org/Public/"
UNICODE_VERSION = "15.1.0"
UNICODE_DATA = "UnicodeData.txt"


class GeneralCategory(Enum):
    Letter = "L"
    LetterUppercase = "Lu"
    LetterLowercase = "Ll"
    LetterTitlecase = "Lt"
    LemmerModifier = "Lm"
    LetterOther = "Lo"

    Mark = "M"
    MarkNonspacing = "Mn"
    MarkSpacingCombining = "Mc"
    MarkEnclosing = "Me"

    Number = "N"
    NumberDecimalDigit = "Nd"
    NumberLetter = "Nl"
    NumberOther = "No"

    Punctuation = "P"
    PunctuationConnector = "Pc"
    PunctuationDash = "Pd"
    PunctuationOpen = "Ps"
    PunctuationClose = "Pe"
    PunctuationInitialQuote = "Pi"
    PunctuationFinalQuote = "Pf"
    PunctuationOther = "Po"

    Symbol = "S"
    SymbolMath = "Sm"
    SymbolCurrency = "Sc"
    SymbolModifier = "Sk"
    SymbolOther = "So"

    Separator = "Z"
    SeparatorSpace = "Zs"
    SeparatorLine = "Zl"
    SeparatorParagraph = "Zp"

    Other = "C"
    OtherControl = "Cc"
    OtherFormat = "Cf"
    OtherSurrogate = "Cs"
    OtherPrivateUse = "Co"
    OtherNoAssigned = "Cn"

@dataclass
class RawRecord:
    codepoint: int
    name: str
    general_category: str
    canonical_combining_class: str
    bidi_class: str
    decomposition_type: str
    decomposition_mapping: list[str]
    decimal: int | None
    digit: int | None
    is_numeric: bool
    bidi_mirrored: str
    unicode_1_name: str
    iso_comment: str
    simple_uppercase_mapping: int | None
    simple_lowercase_mapping: int | None
    simple_titlecase_mapping: int | None


class RecordMask(IntEnum):
    @staticmethod
    def _generate_next_value_(name, start, count, last_values):
        if count == 16:
            assert False, "В тебе тільки 16 біт :D"
        return 2**count

    LOWERCASE = auto()
    UPPERCASE = auto()
    TITLECASE = auto()
    IS_SPACE = auto()
    IS_LETTER = auto()
    IS_DECIMAL = auto()
    IS_DIGIT = auto()
    IS_NUMERIC = auto()


@dataclass
class Record:
    lowercase_offset: int
    uppercase_offset: int
    titlecase_offset: int
    flags: int

    def to_str(self):
        result = "{"
        *other, flags = astuple(self)
        result += ", ".join(map(str, other))
        result += f", {bin(flags)}"
        result += "}"
        return result


class UnicodeDatabase:
    def __init__(self):
        self._chars: list[RawRecord | None] = [None] * 0x110000
        self._index: list[int] = [0] * 0x110000
        self._records: list[Record] = [Record(0, 0, 0 , 0)]  # На першому місці має стояти запис з нулями
        self._generate_chars()
        self._generate_records_and_index()

    def _open_ucd_file(self, filename: str):
        response = request.urlopen(f"{UNICODE_REPO}{UNICODE_VERSION}/ucd/{filename}")
        data = response.read().decode("utf-8")
        return StringIO(data)

    def _parse_hex_int(self, string) -> int | None:
        if string:
            return int(string, 16)
        return None

    def _parse_record(self, string: str) -> RawRecord:
        # https://www.unicode.org/reports/tr44/#UnicodeData.txt - опис формату
        values = string.removesuffix("\n").split(";")
        decomposition_type, *decomposition_mapping = values[5].split() if values[5] else ("",)
        return RawRecord(
            codepoint=int(values[0], 16),
            name=values[1],
            general_category=values[2],
            canonical_combining_class=values[3],
            bidi_class=values[4],
            decomposition_type=decomposition_type,
            decomposition_mapping=decomposition_mapping,
            decimal = None if not values[6] else int(values[6]),
            digit = None if not values[7] else int(values[7]),
            is_numeric = bool(values[8]),
            bidi_mirrored=values[9],
            unicode_1_name=values[10],
            iso_comment=values[11],
            simple_uppercase_mapping=self._parse_hex_int(values[12]),
            simple_lowercase_mapping=self._parse_hex_int(values[13]),
            simple_titlecase_mapping=self._parse_hex_int(values[14]),
        )

    def _generate_chars(self):
        with self._open_ucd_file(UNICODE_DATA) as f:
            while line := f.readline():
                record = self._parse_record(line)
                if record.name.endswith("First>"):
                    record.name = record.name.split(",")[0].removeprefix("<")
                    record2 = self._parse_record(f.readline())
                    for i in range(record.codepoint, record2.codepoint + 1):
                        self._chars[i] = record
                else:
                    self._chars[record.codepoint] = record

    def make_record(self, char: RawRecord | None) -> Record:
        record = Record(0, 0, 0, 0)
        if char is None:
            return record
        if char.general_category == GeneralCategory.LetterUppercase.value:
            record.flags |= RecordMask.UPPERCASE
        if char.general_category == GeneralCategory.LetterLowercase.value:
            record.flags |= RecordMask.LOWERCASE
        if char.general_category == GeneralCategory.LetterTitlecase.value:
            record.flags |= RecordMask.TITLECASE
        if char.simple_lowercase_mapping:
            record.lowercase_offset = char.codepoint - char.simple_lowercase_mapping
        if char.simple_uppercase_mapping:
            record.uppercase_offset = char.codepoint - char.simple_uppercase_mapping
        if char.simple_titlecase_mapping:
            record.titlecase_offset = char.codepoint - char.simple_titlecase_mapping
        if char.general_category == GeneralCategory.SeparatorSpace.value:
            record.flags |= RecordMask.IS_SPACE
        if char.general_category.startswith(GeneralCategory.Letter.value):
            record.flags |= RecordMask.IS_LETTER
        if char.decimal:
            record.flags |= RecordMask.IS_DECIMAL
        if char.digit:
            record.flags |= RecordMask.IS_DIGIT
        if char.is_numeric:
            record.flags |= RecordMask.IS_NUMERIC
        return record

    def _generate_records_and_index(self):
        for i, char in enumerate(self._chars):
            record = self.make_record(char)
            if record not in self._records:
                self._records.append(record)
            self._index[i] = self._records.index(record)
        assert len(self._records) < 255,\
            "Якщо кількість записів більша, то потрібно змінити тип для масиву індексації, зараз використовується unsigned short"

    @property
    def chars(self):
        return self._chars

    @property
    def index(self):
        return self._index

    @property
    def records(self) -> list[Record]:
        return self._records


def rle_compress(input_list):
    rle_values = []
    rle_lenghts = []
    rle_indexes = []

    current_value = input_list[0]
    current_index = 0
    current_length = 1

    for i in range(1, len(input_list)):
        if input_list[i] == current_value:
            current_length += 1
        else:
            rle_values.append(current_value)
            rle_lenghts.append(current_length)
            rle_indexes.append(current_index)

            current_value = input_list[i]
            current_index = i
            current_length = 1

    # Остання група
    rle_values.append(current_value)
    rle_lenghts.append(current_length)
    rle_indexes.append(current_index)

    return rle_values, rle_lenghts, rle_indexes


def print_elements_with_limit(elements, format_func, line_limit=80, indent=4):
    """
    Друкує елементи списку у форматованому вигляді з обмеженням довжини рядка та відступом від лівого краю.
    """
    indent_str = ' ' * indent
    line_length = 4

    print(indent_str, end="")
    for element in elements:
        formatted_element = format_func(element)
        element_len = len(formatted_element)

        if line_length + element_len + (line_length > 0) > line_limit:
            print(",")
            print(indent_str, end="")
            line_length = indent

        if line_length > indent:
            print(", ", end="")

        print(formatted_element, end="")
        line_length += element_len + (line_length > indent)
    print()


def generate_database_file():
    with open(UNICODE_DATABASE_HPP, "w", encoding="utf-8") as f:
        with redirect_stdout(f):
            print("#ifndef UNICODE_DATABASE_HPP")
            print("#define UNICODE_DATABASE_HPP")
            print("")
            print("#include <array>")
            print("#include \"unicode.hpp\"\n")
            print("using namespace unicode;\n")

            for mask in RecordMask:
                print(f"constexpr unsigned short {mask.name} = {hex(mask.value)};")
            print()

            print(f"UnicodeRecord records[{len(UNICODE_DATABASE.records)}] = {{")
            print_elements_with_limit(UNICODE_DATABASE.records, lambda record: record.to_str())
            print("};\n")

            rle_values, _, rle_indexes = rle_compress(UNICODE_DATABASE.index)
            print(f"std::array<unsigned char, {len(rle_values)}> values = {{")
            print_elements_with_limit(rle_values, lambda value: str(value))
            print("};\n")

            print(f"std::array<unsigned int, {len(rle_indexes)}> indexes = {{")
            print_elements_with_limit(rle_indexes, lambda num: str(num))
            print("};\n")

            print("#endif")


UNICODE_DATABASE = UnicodeDatabase()
generate_database_file()
