from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
GAME_MODEL = ROOT / "src" / "gui" / "game" / "GameModel.cpp"


class DeterministicSaveLoadContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.game_model = GAME_MODEL.read_text(encoding="utf-8")

    def test_full_save_load_restores_continuation_state_after_loading(self) -> None:
        for function_name in ("SetSave", "SetSaveFile"):
            start = self.game_model.index(f"void GameModel::{function_name}(")
            end = self.game_model.index("\n}\n", start)
            body = self.game_model[start:end]
            apply_call = "SaveToSimParameters(*saveData);"
            self.assertEqual(body.count(apply_call), 2)

            first_apply = body.index(apply_call)
            clear = body.index("sim->clear_sim();")
            load = body.index("sim->Load(saveData")
            final_apply = body.index(apply_call, first_apply + 1)
            self.assertLess(first_apply, clear)
            self.assertLess(clear, load)
            self.assertLess(load, final_apply)

        self.assertIn("loading particles", self.game_model)
        self.assertIn("advance the RNG", self.game_model)


if __name__ == "__main__":
    unittest.main()
