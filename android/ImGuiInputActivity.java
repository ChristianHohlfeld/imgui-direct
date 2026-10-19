package de.imgui.direct;

import android.app.NativeActivity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputConnectionWrapper;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;

// Platform-only IME bridge: all widgets, state, gesture recognition and rendering
// remain in C++/Dear ImGui. NativeActivity still owns the native game loop.
public final class ImGuiInputActivity extends NativeActivity {
    static { System.loadLibrary("imgui_direct"); }
    private static native void nativeText(String utf8);
    private static native void nativeBackspace();
    private EditText editor;

    @Override public void onCreate(Bundle saved) {
        super.onCreate(saved);
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        editor = new EditText(this) {
            @Override public InputConnection onCreateInputConnection(EditorInfo info) {
                InputConnection base=super.onCreateInputConnection(info);
                if(base==null)return null;
                return new InputConnectionWrapper(base,false) {
                    @Override public boolean deleteSurroundingText(int before,int after) {
                        if(getText().length()==0 && before>0 && after==0) {
                            nativeBackspace();
                            return true;
                        }
                        return super.deleteSurroundingText(before,after);
                    }
                };
            }
            @Override public boolean onKeyDown(int key, KeyEvent event) {
                if(key==KeyEvent.KEYCODE_DEL && getText().length()==0) {
                    nativeBackspace(); return true;
                }
                return super.onKeyDown(key,event);
            }
        };
        editor.setBackgroundColor(android.graphics.Color.TRANSPARENT);
        editor.setTextColor(android.graphics.Color.TRANSPARENT);
        editor.setAlpha(0.02f);
        editor.setSingleLine(true);
        editor.setInputType(android.text.InputType.TYPE_CLASS_TEXT |
            android.text.InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        editor.setFocusableInTouchMode(true);
        editor.addTextChangedListener(new TextWatcher() {
            public void beforeTextChanged(CharSequence s,int start,int count,int after) {}
            public void onTextChanged(CharSequence s,int start,int before,int count) {}
            public void afterTextChanged(Editable value) {
                if(BaseInputConnection.getComposingSpanStart(value)!=-1)return;
                if(value.length()>0) {
                    nativeText(value.toString());
                    value.clear();
                }
            }
        });
        FrameLayout.LayoutParams params=new FrameLayout.LayoutParams(2,2);
        getWindow().addContentView(editor,params);
        editor.clearFocus();
    }
    public void showKeyboard(boolean show) {
        runOnUiThread(()->{
            InputMethodManager imm=(InputMethodManager)getSystemService(INPUT_METHOD_SERVICE);
            if(show) {
                editor.requestFocus();
                imm.showSoftInput(editor,InputMethodManager.SHOW_IMPLICIT);
            } else {
                imm.hideSoftInputFromWindow(editor.getWindowToken(),0);
                editor.clearFocus();
            }
        });
    }
    public String getClipboardText() {
        ClipboardManager cb=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        if(!cb.hasPrimaryClip()||cb.getPrimaryClip().getItemCount()==0)return "";
        CharSequence text=cb.getPrimaryClip().getItemAt(0).coerceToText(this);
        return text==null?"":text.toString();
    }
    public void setClipboardText(String text) {
        ClipboardManager cb=(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
        cb.setPrimaryClip(ClipData.newPlainText("Text",text));
    }
}
